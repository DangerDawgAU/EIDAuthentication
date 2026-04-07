// File: EIDMigrate/SecurityHelpers.cpp
// Security helper functions that don't depend on tracing or other EIDMigrate-specific features

#include "SecurityHelpers.h"

// Constant-time comparison to prevent timing attacks
// Uses XOR accumulation so comparison time is independent of content
BOOL ConstantTimeEqual(
    _In_reads_bytes_(cbData) const BYTE* pbData1,
    _In_reads_bytes_(cbData) const BYTE* pbData2,
    _In_ DWORD cbData)
{
    if (!pbData1 || !pbData2)
        return FALSE;

    BYTE result = 0;
    for (DWORD i = 0; i < cbData; i++)
    {
        result |= pbData1[i] ^ pbData2[i];
    }
    return result == 0;
}

// Constant-time wide string comparison
BOOL ConstantTimeEqualString(
    _In_ PCWSTR pwszString1,
    _In_ PCWSTR pwszString2)
{
    if (!pwszString1 || !pwszString2)
        return FALSE;

    size_t cchLen1 = wcslen(pwszString1);
    size_t cchLen2 = wcslen(pwszString2);

    // Lengths must match - include in comparison to prevent length oracle
    if (cchLen1 != cchLen2)
        return FALSE;

    // Compare all characters
    return ConstantTimeEqual(
        reinterpret_cast<const BYTE*>(pwszString1),
        reinterpret_cast<const BYTE*>(pwszString2),
        static_cast<DWORD>(cchLen1 * sizeof(WCHAR)));
}
