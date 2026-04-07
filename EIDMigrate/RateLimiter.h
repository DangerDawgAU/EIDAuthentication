// File: EIDMigrate/RateLimiter.h
// Rate limiting for security-sensitive operations (PIN/passphrase attempts)

#pragma once
#include <Windows.h>
#include <vector>

// Forward declaration to avoid circular dependency
// EIDM_TRACE_WARN is defined in Tracing.h, but we include this in PinPrompt.cpp which includes Tracing.h
// For now, use OutputDebugString as a fallback
#ifndef EIDM_TRACE_WARN
#define EIDM_TRACE_WARN(fmt, ...) do { \
    WCHAR szMsg_[512]; \
    swprintf_s(szMsg_, ARRAYSIZE(szMsg_), fmt, __VA_ARGS__); \
    OutputDebugStringW(szMsg_); \
} while(0)
#endif

// Rate limiter to prevent brute force attacks on PIN/passphrase entry
class SecurityRateLimiter
{
public:
    // Singleton instance
    static SecurityRateLimiter& GetInstance()
    {
        static SecurityRateLimiter instance;
        return instance;
    }

    // Record a failed authentication attempt
    // Returns TRUE if the operation should be blocked (too many failures)
    // Also outputs the recommended delay in milliseconds
    BOOL RecordFailedAttempt(_In_opt_ PCWSTR pwszOperationName, _Out_ DWORD* pdwDelayMs = nullptr)
    {
        SYSTEMTIME stNow;
        GetSystemTime(&stNow);
        ULONGLONG ullNow = SystemTimeToULL(stNow);

        // Use a simple counter based on time window
        const ULONGLONG FAILURE_WINDOW_MS = 300000;  // 5 minutes
        const DWORD MAX_FAILURES = 5;                 // Max failures in window

        // Clean old entries
        CleanupOldEntries(ullNow, FAILURE_WINDOW_MS);

        // Add new failure
        m_failureAttempts.push_back(ullNow);

        // Calculate delay with exponential backoff
        DWORD dwFailureCount = static_cast<DWORD>(m_failureAttempts.size());
        DWORD dwDelayMs = 0;

        if (dwFailureCount > MAX_FAILURES)
        {
            // Exponential backoff: 2^(failures - MAX_FAILURES) seconds
            DWORD dwExponent = dwFailureCount - MAX_FAILURES;
            if (dwExponent > 10) dwExponent = 10;  // Cap at ~17 minutes
            dwDelayMs = (1000 << dwExponent);  // 2^exponent seconds in ms
        }

        if (pdwDelayMs)
            *pdwDelayMs = dwDelayMs;

        if (dwDelayMs > 0)
        {
            if (pwszOperationName)
            {
                EIDM_TRACE_WARN(L"Rate limit: %u failed attempts for '%ls'. Delay: %u ms",
                    dwFailureCount, pwszOperationName, dwDelayMs);
            }
            return TRUE;  // Should block
        }

        return FALSE;  // Allow to proceed
    }

    // Record a successful attempt (clears the failure counter)
    void RecordSuccess()
    {
        m_failureAttempts.clear();
    }

    // Get the number of recent failures
    DWORD GetRecentFailureCount()
    {
        return static_cast<DWORD>(m_failureAttempts.size());
    }

private:
    SecurityRateLimiter() = default;
    ~SecurityRateLimiter() = default;

    // Disable copy
    SecurityRateLimiter(const SecurityRateLimiter&) = delete;
    SecurityRateLimiter& operator=(const SecurityRateLimiter&) = delete;

    // Convert SYSTEMTIME to ULONGLONG (100ns units since Jan 1, 1601)
    static ULONGLONG SystemTimeToULL(_In_ const SYSTEMTIME& st)
    {
        FILETIME ft;
        SystemTimeToFileTime(&st, &ft);
        return (static_cast<ULONGLONG>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    }

    // Remove old entries outside the time window
    void CleanupOldEntries(_In_ ULONGLONG ullNow, _In_ ULONGLONG ullWindowMs)
    {
        ULONGLONG ullWindow100ns = ullWindowMs * 10000;  // Convert ms to 100ns units

        auto it = m_failureAttempts.begin();
        while (it != m_failureAttempts.end())
        {
            if (ullNow - *it > ullWindow100ns)
            {
                it = m_failureAttempts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    std::vector<ULONGLONG> m_failureAttempts;  // Timestamps of failed attempts
};
