#pragma once

// File: EIDMigrate/SecurityHelpers.h
// Security helper functions that don't depend on tracing or other EIDMigrate-specific features

#include <Windows.h>

// Constant-time comparison to prevent timing attacks on passwords/PINs
// Returns TRUE if buffers are equal, FALSE otherwise
// Always processes entire buffer regardless of early mismatches
BOOL ConstantTimeEqual(
    _In_reads_bytes_(cbData) const BYTE* pbData1,
    _In_reads_bytes_(cbData) const BYTE* pbData2,
    _In_ DWORD cbData);

// Constant-time wide string comparison for password/PIN verification
// Returns TRUE if strings are equal, FALSE otherwise
BOOL ConstantTimeEqualString(
    _In_ PCWSTR pwszString1,
    _In_ PCWSTR pwszString2);
