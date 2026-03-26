#pragma once

// File: EIDMigrate/PinPrompt.h
// Smart card PIN prompt dialog

#include "SecureMemory.h"
#include <Windows.h>

// PIN prompt result
enum class PIN_PROMPT_RESULT
{
    SUCCESS,
    CANCELLED,
    PIN_ERROR,  // Renamed from ERROR to avoid Windows macro conflict
};

// Secure PIN prompt dialog
class SecurePinPrompt
{
private:
    HWND m_hDialog;
    HWND m_hPinEdit;
    HWND m_hPromptLabel;
    HWND m_hOkButton;
    HWND m_hCancelButton;
    SecurePin m_pin;
    PIN_PROMPT_RESULT m_result;
    std::wstring wsPromptText;

public:
    SecurePinPrompt();
    ~SecurePinPrompt();

    // Delete copy
    SecurePinPrompt(const SecurePinPrompt&) = delete;
    SecurePinPrompt& operator=(const SecurePinPrompt&) = delete;

    // Show PIN prompt dialog
    PIN_PROMPT_RESULT ShowPrompt(
        _In_opt_ PCWSTR pwszPromptText,
        _Out_ SecurePin& pin);

    // Set custom prompt text
    void SetPromptText(_In_ PCWSTR pwszPromptText);

private:
    static INT_PTR CALLBACK PinDialogProc(
        _In_ HWND hDlg,
        _In_ UINT uMsg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);

    BOOL OnInitDialog(_In_ HWND hDlg);
    void OnOk(_In_ HWND hDlg);
    void OnCancel(_In_ HWND hDlg);
};

// Convenience function to prompt for PIN
PIN_PROMPT_RESULT PromptForPIN(
    _In_opt_ PCWSTR pwszPromptText,
    _Out_ SecurePin& pin);

// Prompt for passphrase with confirmation
PIN_PROMPT_RESULT PromptForPassphraseWithConfirm(
    _Out_ SecurePassphrase& passphrase,
    _In_ DWORD minLength = 16);

// Validate PIN format (typically 4-8 digits for smart cards)
BOOL IsValidPinFormat(_In_ PCWSTR pwszPin);

// Validate PIN length
BOOL IsValidPinLength(_In_ SIZE_T cchPin);
