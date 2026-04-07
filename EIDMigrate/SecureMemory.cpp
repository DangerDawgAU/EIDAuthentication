// File: EIDMigrate/SecureMemory.cpp
// Secure memory management classes implementation

#include "SecureMemory.h"
#include "Tracing.h"
#include <algorithm>

// SecureBuffer implementation
SecureBuffer::SecureBuffer() : m_pbData(nullptr), m_cbSize(0), m_fLocked(FALSE)
{
}

SecureBuffer::SecureBuffer(DWORD cbSize) : m_pbData(nullptr), m_cbSize(cbSize), m_fLocked(FALSE)
{
    m_pbData = static_cast<PBYTE>(malloc(cbSize)); // NOSONAR - malloc used for secure memory, compatible with SecureZeroMemory
    if (!m_pbData)
        throw std::bad_alloc();

    SecureZeroMemory(m_pbData, m_cbSize);
}

SecureBuffer::~SecureBuffer()
{
    if (m_pbData)
    {
        if (m_fLocked)
            Unlock();
        SecureZeroMemory(m_pbData, m_cbSize);
        free(m_pbData); // NOSONAR - free used for secure memory, matching malloc
    }
}

SecureBuffer::SecureBuffer(SecureBuffer&& other) noexcept :
    m_pbData(other.m_pbData),
    m_cbSize(other.m_cbSize),
    m_fLocked(other.m_fLocked)
{
    other.m_pbData = nullptr;
    other.m_cbSize = 0;
    other.m_fLocked = FALSE;
}

SecureBuffer& SecureBuffer::operator=(SecureBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (m_pbData)
        {
            if (m_fLocked)
                Unlock();
            SecureZeroMemory(m_pbData, m_cbSize);
            free(m_pbData); // NOSONAR - free used for secure memory, matching malloc
        }

        m_pbData = other.m_pbData;
        m_cbSize = other.m_cbSize;
        m_fLocked = other.m_fLocked;

        other.m_pbData = nullptr;
        other.m_cbSize = 0;
        other.m_fLocked = FALSE;
    }
    return *this;
}

BOOL SecureBuffer::Lock()
{
    if (m_pbData && m_cbSize > 0 && !m_fLocked)
    {
        m_fLocked = VirtualLock(m_pbData, m_cbSize);
        return m_fLocked;
    }
    return TRUE;
}

void SecureBuffer::Unlock()
{
    if (m_pbData && m_cbSize > 0 && m_fLocked)
    {
        VirtualUnlock(m_pbData, m_cbSize);
        m_fLocked = FALSE;
    }
}

BOOL SecureBuffer::Resize(DWORD cbNewSize)
{
    if (cbNewSize == m_cbSize)
        return TRUE;

    if (m_fLocked)
        Unlock();

    // BUG FIX #17: Memory exhaustion protection
    // Maximum reasonable secure buffer size (prevents exhaustion attacks)
    constexpr DWORD MAX_SECURE_BUFFER_SIZE = 16 * 1024 * 1024;  // 16 MB
    if (cbNewSize > MAX_SECURE_BUFFER_SIZE)
    {
        EIDM_TRACE_ERROR(L"Requested secure buffer size %u bytes exceeds maximum %u bytes",
            cbNewSize, MAX_SECURE_BUFFER_SIZE);
        return FALSE;
    }

    PBYTE pbNewData = static_cast<PBYTE>(realloc(m_pbData, cbNewSize)); // NOSONAR - realloc used for secure memory; preserves data while resizing for SecureZeroMemory compatibility
    if (!pbNewData && cbNewSize > 0)
        return FALSE;

    m_pbData = pbNewData;

    // Zero new memory if growing
    if (cbNewSize > m_cbSize)
    {
        SecureZeroMemory(m_pbData + m_cbSize, cbNewSize - m_cbSize);
    }

    m_cbSize = cbNewSize;
    return TRUE;
}

void SecureBuffer::Clear()
{
    if (m_pbData)
    {
        SecureZeroMemory(m_pbData, m_cbSize);
    }
}

// SecurePin implementation
SecurePin::SecurePin() : m_wsPin()
{
}

SecurePin::SecurePin(PCWSTR pwszPin) : m_wsPin()
{
    if (pwszPin && *pwszPin)
    {
        m_wsPin = SecureWString(pwszPin);
    }
}

SecurePin::~SecurePin()
{
    if (!m_wsPin.empty())
    {
        m_wsPin.clear();
    }
}

SecurePin::SecurePin(SecurePin&& other) noexcept : m_wsPin(std::move(other.m_wsPin))
{
}

SecurePin& SecurePin::operator=(SecurePin&& other) noexcept
{
    if (this != &other)
    {
        if (!m_wsPin.empty())
        {
            m_wsPin.clear();
        }
        m_wsPin = std::move(other.m_wsPin);
    }
    return *this;
}

void SecurePin::Set(_In_z_ PCWSTR pwszPin)
{
    if (!m_wsPin.empty())
    {
        m_wsPin.clear();
    }

    if (pwszPin && *pwszPin)
    {
        m_wsPin = SecureWString(pwszPin);
    }
}

void SecurePin::Clear()
{
    if (!m_wsPin.empty())
    {
        m_wsPin.clear();
    }
}

// CryptoOperationScope implementation
PBYTE CryptoOperationScope::AllocateBuffer(DWORD cbSize)
{
    m_sensitiveBuffers.emplace_back(cbSize);
    return m_sensitiveBuffers.back().data();
}

PWSTR CryptoOperationScope::AllocateString(size_t cchChars)
{
    m_sensitiveStrings.emplace_back();
    m_sensitiveStrings.back().reserve(cchChars);
    return const_cast<PWSTR>(m_sensitiveStrings.back().c_str());
}

void CryptoOperationScope::Clear()
{
    m_sensitiveBuffers.clear();
    m_sensitiveStrings.clear();
}

// Secure memory utility functions
void SecureFree(_In_ PVOID pvMem, _In_ SIZE_T cbSize) // NOSONAR - PVOID (void*) is standard Windows API type for generic pointers
{
    if (pvMem && cbSize > 0)
    {
        SecureZeroMemory(pvMem, cbSize);
        free(pvMem); // NOSONAR - free used for secure memory, matching malloc
    }
}

PVOID SecureAlloc(_In_ SIZE_T cbSize) // NOSONAR - PVOID (void*) required for Windows API compatibility with secure memory functions
{
    // BUG FIX #17: Memory exhaustion protection
    // Maximum reasonable single allocation (prevents exhaustion attacks)
    constexpr SIZE_T MAX_SECURE_ALLOC_SIZE = 16 * 1024 * 1024;  // 16 MB
    if (cbSize > MAX_SECURE_ALLOC_SIZE)
    {
        EIDM_TRACE_ERROR(L"Requested secure allocation size %zu bytes exceeds maximum %zu bytes",
            cbSize, MAX_SECURE_ALLOC_SIZE);
        return nullptr;
    }

    PVOID pvMem = malloc(cbSize); // NOSONAR - malloc used for secure memory, compatible with SecureZeroMemory
    if (pvMem)
    {
        SecureZeroMemory(pvMem, cbSize);
    }
    return pvMem;
}
