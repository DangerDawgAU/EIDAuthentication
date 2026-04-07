// File: EIDMigrate/PinPrompt.cpp
// Smart card PIN prompt dialog

#include "PinPrompt.h"
#include "RateLimiter.h"
#include "resource.h"
#include "Tracing.h"
#include "Utils.h"
#include <Windowsx.h>
#include <thread>

SecurePinPrompt::SecurePinPrompt() :
    m_hDialog(nullptr),
    m_hPinEdit(nullptr),
    m_hPromptLabel(nullptr),
    m_hOkButton(nullptr),
    m_hCancelButton(nullptr),
    m_result(PIN_PROMPT_RESULT::PIN_ERROR),
    wsPromptText(L"Enter PIN:")
{
}

SecurePinPrompt::~SecurePinPrompt()
{
    if (m_hDialog)
    {
        DestroyWindow(m_hDialog);
    }
}

void SecurePinPrompt::SetPromptText(_In_ PCWSTR pwszPromptText)
{
    if (pwszPromptText)
        wsPromptText = pwszPromptText;
}

PIN_PROMPT_RESULT SecurePinPrompt::ShowPrompt(_In_opt_ PCWSTR pwszPromptText, _Out_ SecurePin& pin)
{
    if (pwszPromptText)
        wsPromptText = pwszPromptText;

    m_result = PIN_PROMPT_RESULT::PIN_ERROR;

    INT_PTR iResult = DialogBoxParamW(
        GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDD_PIN_PROMPT),
        nullptr,
        PinDialogProc,
        reinterpret_cast<LPARAM>(this));

    if (iResult == IDOK && m_result == PIN_PROMPT_RESULT::SUCCESS)
    {
        pin = std::move(m_pin);
    }

    return m_result;
}

INT_PTR CALLBACK SecurePinPrompt::PinDialogProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        SetWindowLongPtrW(hDlg, GWLP_USERDATA, lParam);
        SecurePinPrompt* pThis = reinterpret_cast<SecurePinPrompt*>(lParam);
        pThis->m_hDialog = hDlg;
        return pThis->OnInitDialog(hDlg);
    }

    SecurePinPrompt* pThis = reinterpret_cast<SecurePinPrompt*>(
        GetWindowLongPtrW(hDlg, GWLP_USERDATA));

    if (!pThis)
        return FALSE;

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            pThis->OnOk(hDlg);
            return TRUE;

        case IDCANCEL:
            pThis->OnCancel(hDlg);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL SecurePinPrompt::OnInitDialog(_In_ HWND hDlg)
{
    // Get controls
    m_hPinEdit = GetDlgItem(hDlg, IDC_PIN_EDIT);
    m_hPromptLabel = GetDlgItem(hDlg, IDC_PIN_PROMPT_LABEL);
    m_hOkButton = GetDlgItem(hDlg, IDOK);
    m_hCancelButton = GetDlgItem(hDlg, IDCANCEL);

    // Set prompt text
    if (m_hPromptLabel)
    {
        SetWindowTextW(m_hPromptLabel, wsPromptText.c_str());
    }

    // Configure PIN edit control
    if (m_hPinEdit)
    {
        SendMessageW(m_hPinEdit, EM_SETPASSWORDCHAR, L'*', 0);
        SendMessageW(m_hPinEdit, EM_SETLIMITTEXT, 12, 0);
    }

    // Center dialog
    CenterWindow(hDlg);

    // Set focus to PIN edit
    SetFocus(m_hPinEdit);

    return FALSE; // We set the focus
}

void SecurePinPrompt::OnOk(_In_ HWND hDlg)
{
    if (!m_hPinEdit)
    {
        EndDialog(hDlg, IDCANCEL);
        m_result = PIN_PROMPT_RESULT::PIN_ERROR;
        return;
    }

    // Get PIN length
    DWORD cchPin = static_cast<DWORD>(SendMessageW(m_hPinEdit, WM_GETTEXTLENGTH, 0, 0));

    if (cchPin == 0)
    {
        // Empty PIN is invalid
        return;
    }

    // BUG FIX #17: Integer overflow protection
    // Maximum reasonable PIN length (prevents overflow and memory exhaustion)
    constexpr DWORD MAX_PIN_LENGTH = 256;  // Well above any reasonable PIN
    if (cchPin > MAX_PIN_LENGTH)
    {
        EIDM_TRACE_ERROR(L"PIN exceeds maximum allowed length of %u characters", MAX_PIN_LENGTH);
        EndDialog(hDlg, IDCANCEL);
        m_result = PIN_PROMPT_RESULT::PIN_ERROR;
        return;
    }

    // Check for integer overflow before allocation: (cchPin + 1) * sizeof(WCHAR)
    // Since cchPin <= MAX_PIN_LENGTH (256), and sizeof(WCHAR) is 2, the max is 257 * 2 = 514 bytes
    // This cannot overflow even on 32-bit systems (max DWORD is 4,294,967,295)
    DWORD cbPin = (cchPin + 1) * sizeof(WCHAR);

    // Get PIN text
    PWSTR pwszPin = static_cast<PWSTR>(malloc(cbPin));
    if (!pwszPin)
    {
        EndDialog(hDlg, IDCANCEL);
        m_result = PIN_PROMPT_RESULT::PIN_ERROR;
        return;
    }

    SendMessageW(m_hPinEdit, WM_GETTEXT, cchPin + 1, reinterpret_cast<LPARAM>(pwszPin));

    // Store in secure object
    m_pin = SecurePin(pwszPin);

    // Zero temporary buffer
    SecureZeroMemory(pwszPin, cbPin);
    free(pwszPin);

    m_result = PIN_PROMPT_RESULT::SUCCESS;
    EndDialog(hDlg, IDOK);
}

void SecurePinPrompt::OnCancel(_In_ HWND hDlg)
{
    m_result = PIN_PROMPT_RESULT::CANCELLED;
    EndDialog(hDlg, IDCANCEL);
}

// Convenience function
PIN_PROMPT_RESULT PromptForPIN(_In_opt_ PCWSTR pwszPromptText, _Out_ SecurePin& pin)
{
    // BUG FIX #20: Rate limiting for PIN attempts to prevent brute force attacks
    SecurityRateLimiter& rateLimiter = SecurityRateLimiter::GetInstance();

    // Check if we should rate limit
    DWORD dwDelayMs = 0;
    if (rateLimiter.RecordFailedAttempt(L"PIN entry", &dwDelayMs))
    {
        EIDM_TRACE_WARN(L"Too many failed PIN attempts. Waiting %u ms before allowing retry.", dwDelayMs);
        Sleep(dwDelayMs);
    }

    SecurePinPrompt prompt;
    PIN_PROMPT_RESULT result = prompt.ShowPrompt(pwszPromptText, pin);

    // Record success or failure
    if (result == PIN_PROMPT_RESULT::SUCCESS)
    {
        rateLimiter.RecordSuccess();
    }

    return result;
}

// Prompt for passphrase with confirmation
PIN_PROMPT_RESULT PromptForPassphraseWithConfirm(_Out_ SecurePassphrase& passphrase, _In_ DWORD minLength)
{
    const DWORD MAX_PASSPHRASE_LENGTH = 256;

    // First passphrase prompt
    EIDM_TRACE_INFO(L"Enter passphrase (min %u characters):", minLength);

    std::vector<WCHAR> wsPassword1;
    std::vector<WCHAR> wsPassword2;

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE)
    {
        return PIN_PROMPT_RESULT::PIN_ERROR;
    }

    // Set console mode to disable echo
    DWORD dwMode = 0;
    GetConsoleMode(hStdin, &dwMode);
    SetConsoleMode(hStdin, dwMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

    // Read first passphrase with masking
    wprintf(L"Enter passphrase: ");
    for (;;)
    {
        WCHAR ch = L'\0';
        DWORD dwRead = 0;
        if (!ReadConsoleW(hStdin, &ch, 1, &dwRead, nullptr) || dwRead == 0)
        {
            break;
        }

        if (ch == L'\r' || ch == L'\n')
        {
            wprintf(L"\n");
            break;
        }

        if (ch == L'\b')  // Backspace
        {
            if (!wsPassword1.empty())
            {
                wsPassword1.pop_back();
                wprintf(L"\b \b");  // Erase asterisk
            }
            continue;
        }

        if (ch == 3)  // Ctrl+C
        {
            wprintf(L"\n");
            SetConsoleMode(hStdin, dwMode);
            return PIN_PROMPT_RESULT::CANCELLED;
        }

        if (wsPassword1.size() < MAX_PASSPHRASE_LENGTH - 1)
        {
            wsPassword1.push_back(ch); // NOSONAR - push_back used for primitive type (WCHAR); emplace_back provides no benefit
            wprintf(L"*");  // Mask character
        }
    }

    // Restore console mode
    SetConsoleMode(hStdin, dwMode);

    // Check minimum length
    if (wsPassword1.size() < minLength)
    {
        EIDM_TRACE_ERROR(L"Passphrase must be at least %u characters.", minLength);
        return PIN_PROMPT_RESULT::PIN_ERROR;
    }

    // Confirm passphrase
    wprintf(L"Confirm passphrase: ");

    // Set console mode to disable echo again
    GetConsoleMode(hStdin, &dwMode);
    SetConsoleMode(hStdin, dwMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

    for (;;)
    {
        WCHAR ch = L'\0';
        DWORD dwRead = 0;
        if (!ReadConsoleW(hStdin, &ch, 1, &dwRead, nullptr) || dwRead == 0)
        {
            break;
        }

        if (ch == L'\r' || ch == L'\n')
        {
            wprintf(L"\n");
            break;
        }

        if (ch == L'\b')  // Backspace
        {
            if (!wsPassword2.empty())
            {
                wsPassword2.pop_back();
                wprintf(L"\b \b");  // Erase asterisk
            }
            continue;
        }

        if (ch == 3)  // Ctrl+C
        {
            wprintf(L"\n");
            SetConsoleMode(hStdin, dwMode);
            return PIN_PROMPT_RESULT::CANCELLED;
        }

        if (wsPassword2.size() < MAX_PASSPHRASE_LENGTH - 1)
        {
            wsPassword2.push_back(ch); // NOSONAR - push_back used for primitive type (WCHAR); emplace_back provides no benefit
            wprintf(L"*");  // Mask character
        }
    }

    // Restore console mode
    SetConsoleMode(hStdin, dwMode);

    // Check if passphrases match
    if (wsPassword1.size() != wsPassword2.size() ||
        !std::equal(wsPassword1.begin(), wsPassword1.end(), wsPassword2.begin()))
    {
        EIDM_TRACE_ERROR(L"Passphrases do not match.");
        return PIN_PROMPT_RESULT::PIN_ERROR;
    }

    // Store in secure object
    wsPassword1.push_back(L'\0');  // Null terminate
    passphrase = SecurePassphrase(wsPassword1.data());

    // Clear temporary storage
    SecureZeroMemory(wsPassword1.data(), wsPassword1.size() * sizeof(WCHAR));
    SecureZeroMemory(wsPassword2.data(), wsPassword2.size() * sizeof(WCHAR));

    return PIN_PROMPT_RESULT::SUCCESS;
}

// Validate PIN format
BOOL IsValidPinFormat(_In_ PCWSTR pwszPin)
{
    if (!pwszPin || *pwszPin == L'\0')
        return FALSE;

    // PIN should be 4-8 digits
    size_t cchLen = wcslen(pwszPin); // NOSONAR - pointer validated for NULL above (line 325)
    if (cchLen < 4 || cchLen > 8)
        return FALSE;

    // All characters should be digits
    for (size_t i = 0; i < cchLen; i++)
    {
        if (!iswdigit(pwszPin[i]))
            return FALSE;
    }

    return TRUE;
}

// Validate PIN length
BOOL IsValidPinLength(_In_ SIZE_T cchPin)
{
    return (cchPin >= 4 && cchPin <= 8);
}
