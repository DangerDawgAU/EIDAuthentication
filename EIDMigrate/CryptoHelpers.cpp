// File: EIDMigrate/CryptoHelpers.cpp
// Cryptographic helper functions implementation

#include "CryptoHelpers.h"
#include "Tracing.h"
#include <vector>
#include <stdexcept>
#include <ntstatus.h>

// Define STATUS constants if not available
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0x00000000
#endif
#ifndef STATUS_UNSUCCESSFUL
#define STATUS_UNSUCCESSFUL 0xC0000001
#endif
#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER 0xC000000D
#endif
#ifndef STATUS_NO_MEMORY
#define STATUS_NO_MEMORY 0xC0000017
#endif

// Define missing CNG constants
#ifndef BCRYPT_HASH_INTERFACE_HMAC
#define BCRYPT_HASH_INTERFACE_HMAC 0x00000001
#endif

// Define BCRYPT_INIT_AUTH_MODE_INFO if not available
#ifndef BCRYPT_INIT_AUTH_MODE_INFO
#define BCRYPT_INIT_AUTH_MODE_INFO(_auth_info_) \
    ((_auth_info_).pbNonce = nullptr, \
     (_auth_info_).cbNonce = 0, \
     (_auth_info_).pbAuthData = nullptr, \
     (_auth_info_).cbAuthData = 0, \
     (_auth_info_).pbTag = nullptr, \
     (_auth_info_).cbTag = 0, \
     (_auth_info_).pbMacContext = nullptr, \
     (_auth_info_).cbMacContext = 0, \
     (_auth_info_).dwFlags = 0)
#endif

// Manual PBKDF2-HMAC-SHA256 implementation
// Since BCryptDeriveKeyPBKDF2 has compatibility issues, we implement PBKDF2 manually

// HMAC-SHA-256 using raw SHA-256 (manual HMAC construction)
// HMAC(K, m) = H((K ^ opad) || H((K ^ ipad) || m))
static CRYPTO_STATUS ComputeHMACSha256(
    _In_reads_bytes_(cbKey) const BYTE* pbKey,
    _In_ DWORD cbKey,
    _In_reads_bytes_(cbData) const BYTE* pbData,
    _In_ DWORD cbData,
    _Out_writes_all_(32) BYTE* pbHash)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    PBYTE pbHashObject = nullptr;
    DWORD cbHashObject = 0;
    DWORD cbResult = 0;
    CRYPTO_STATUS result = CRYPTO_STATUS::CRYPTO_ERROR_KDF_FAILED;

    // HMAC-SHA256 constants
    constexpr BYTE IPAD = 0x36;
    constexpr BYTE OPAD = 0x5c;
    constexpr DWORD SHA256_BLOCK_SIZE = 64;
    constexpr DWORD SHA256_HASH_SIZE = 32;

    // Use stack allocation for fixed-size buffers (no C++ objects)
    BYTE ipadKey[SHA256_BLOCK_SIZE];
    BYTE opadKey[SHA256_BLOCK_SIZE];
    BYTE actualKey[SHA256_HASH_SIZE];  // Max key size after hashing
    DWORD cbActualKey = cbKey;

    // Initialize ipad and opad
    memset(ipadKey, IPAD, SHA256_BLOCK_SIZE);
    memset(opadKey, OPAD, SHA256_BLOCK_SIZE);

    // Open SHA256 algorithm provider
    status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"BCryptOpenAlgorithmProvider failed: 0x%08X", status);
        goto cleanup;
    }

    status = BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH,
        (PBYTE)&cbHashObject, sizeof(DWORD), &cbResult, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        EIDM_TRACE_ERROR(L"BCryptGetProperty OBJECT_LENGTH failed: 0x%08X", status);
        goto cleanup;
    }

    pbHashObject = static_cast<PBYTE>(malloc(cbHashObject)); // NOSONAR - BCrypt API requires malloc/free for hash object buffer
    if (!pbHashObject)
    {
        status = STATUS_NO_MEMORY;
        goto cleanup;
    }

    // Prepare key: if key is longer than block size, hash it first
    if (cbKey > SHA256_BLOCK_SIZE)
    {
        BCRYPT_HASH_HANDLE hKeyHash = NULL;
        BYTE keyHash[SHA256_HASH_SIZE];
        status = BCryptCreateHash(hAlgorithm, &hKeyHash, pbHashObject, cbHashObject, nullptr, 0, 0);
        if (!BCRYPT_SUCCESS(status)) goto cleanup;
        status = BCryptHashData(hKeyHash, reinterpret_cast<PUCHAR>(const_cast<BYTE*>(pbKey)), cbKey, 0);
        if (!BCRYPT_SUCCESS(status)) { BCryptDestroyHash(hKeyHash); goto cleanup; }
        status = BCryptFinishHash(hKeyHash, keyHash, SHA256_HASH_SIZE, 0);
        BCryptDestroyHash(hKeyHash);
        if (!BCRYPT_SUCCESS(status)) goto cleanup;
        memcpy(actualKey, keyHash, SHA256_HASH_SIZE);
        cbActualKey = SHA256_HASH_SIZE;
    }
    else
    {
        memcpy(actualKey, pbKey, cbKey);
    }

    // XOR key with ipad and opad
    for (DWORD i = 0; i < cbActualKey; i++)
    {
        ipadKey[i] ^= actualKey[i];
        opadKey[i] ^= actualKey[i];
    }

    // Inner hash: H((K ^ ipad) || data)
    BYTE innerHash[SHA256_HASH_SIZE];
    status = BCryptCreateHash(hAlgorithm, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptHashData(hHash, ipadKey, SHA256_BLOCK_SIZE, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptHashData(hHash, const_cast<PUCHAR>(pbData), cbData, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptFinishHash(hHash, innerHash, SHA256_HASH_SIZE, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    BCryptDestroyHash(hHash);
    hHash = NULL;

    // Outer hash: H((K ^ opad) || innerHash)
    status = BCryptCreateHash(hAlgorithm, &hHash, pbHashObject, cbHashObject, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptHashData(hHash, opadKey, SHA256_BLOCK_SIZE, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptHashData(hHash, innerHash, SHA256_HASH_SIZE, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    status = BCryptFinishHash(hHash, pbHash, SHA256_HASH_SIZE, 0);
    if (!BCRYPT_SUCCESS(status)) goto cleanup;

    result = CRYPTO_STATUS::CRYPTO_SUCCESS;

cleanup:
    if (pbHashObject) free(pbHashObject); // NOSONAR - memory allocated with malloc for BCrypt API compatibility
    if (hHash) BCryptDestroyHash(hHash);
    if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);

    return result;
}

// XOR two blocks (for HMAC inner/outer padding)
static void XORBlock(_Out_writes_all_(cbSize) BYTE* pbDest, _In_ const BYTE* pbSrc, _In_ BYTE bPad, _In_ DWORD cbSize)
{
    for (DWORD i = 0; i < cbSize; i++)
        pbDest[i] = pbSrc[i] ^ bPad;
}

// PBKDF2-HMAC-SHA256 implementation (RFC 2898)
static CRYPTO_STATUS PBKDF2HMACSHA256(
    _In_reads_bytes_(cbPassword) const BYTE* pbPassword,
    _In_ DWORD cbPassword,
    _In_reads_bytes_(cbSalt) const BYTE* pbSalt,
    _In_ DWORD cbSalt,
    _In_ DWORD cIterations,
    _In_ DWORD cbDerivedKey,
    _Out_writes_all_(cbDerivedKey) BYTE* pbDerivedKey)
{
    // HMAC-SHA256 produces 32 bytes
    constexpr DWORD HASH_LEN = 32;

    EIDM_TRACE_VERBOSE(L"PBKDF2: password=%u bytes, salt=%u bytes, iter=%u, out=%u bytes",
        cbPassword, cbSalt, cIterations, cbDerivedKey);

    // For each block of derived key
    for (DWORD blockIndex = 1; blockIndex <= (cbDerivedKey + HASH_LEN - 1) / HASH_LEN; blockIndex++)
    {
        BYTE blockHash[HASH_LEN];
        BYTE u1[HASH_LEN];
        BYTE saltWithCounter[128];  // Salt + 4-byte counter (big-endian)

        // Prepare salt || INT_32_BE(i)
        DWORD cbSaltWithCounter = cbSalt + 4;
        if (cbSaltWithCounter > sizeof(saltWithCounter))
            return CRYPTO_STATUS::CRYPTO_ERROR_INSUFFICIENT_BUFFER;

        memcpy(saltWithCounter, pbSalt, cbSalt);
        // Write block index in big-endian
        saltWithCounter[cbSalt] = (blockIndex >> 24) & 0xFF;
        saltWithCounter[cbSalt + 1] = (blockIndex >> 16) & 0xFF;
        saltWithCounter[cbSalt + 2] = (blockIndex >> 8) & 0xFF;
        saltWithCounter[cbSalt + 3] = blockIndex & 0xFF;

        EIDM_TRACE_VERBOSE(L"PBKDF2 block %u: computing U1 (salt+%u bytes)", blockIndex, cbSaltWithCounter);

        // U1 = PRF(password, salt || INT_32_BE(i))
        CRYPTO_STATUS cryptoStatus = ComputeHMACSha256(pbPassword, cbPassword, saltWithCounter, cbSaltWithCounter, u1);
        if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
        {
            EIDM_TRACE_ERROR(L"PBKDF2: U1 computation failed for block %u", blockIndex);
            return cryptoStatus;
        }

        // U_{i+1} = PRF(password, U_i)
        memcpy(blockHash, u1, HASH_LEN);

        for (DWORD iter = 1; iter < cIterations; iter++)
        {
            BYTE uNext[HASH_LEN];
            cryptoStatus = ComputeHMACSha256(pbPassword, cbPassword, blockHash, HASH_LEN, uNext);
            if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
            {
                EIDM_TRACE_ERROR(L"PBKDF2: U_%u computation failed for block %u", iter + 1, blockIndex);
                return cryptoStatus;
            }

            // blockHash = blockHash XOR uNext
            for (DWORD i = 0; i < HASH_LEN; i++)
                blockHash[i] ^= uNext[i];
        }

        // Copy to output (possibly partial block)
        DWORD cbBlockCopy = min(HASH_LEN, cbDerivedKey - (blockIndex - 1) * HASH_LEN);
        memcpy(pbDerivedKey + (blockIndex - 1) * HASH_LEN, blockHash, cbBlockCopy);
    }

    return CRYPTO_STATUS::CRYPTO_SUCCESS;
}

CRYPTO_STATUS DeriveKeyFromPassphrase(
    _In_ PCWSTR pwszPassphrase,
    _In_ SIZE_T cchPassphrase,
    _Out_ DERIVED_KEY* pDerivedKey)
{
    if (!pwszPassphrase || cchPassphrase == 0 || !pDerivedKey)
    {
        return CRYPTO_STATUS::CRYPTO_ERROR_INVALID_PARAMETER;
    }

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hRng = NULL;
    CRYPTO_STATUS cryptoStatus = CRYPTO_STATUS::CRYPTO_SUCCESS;

    __try
    {
        // Generate random salt
        status = BCryptOpenAlgorithmProvider(&hRng, BCRYPT_RNG_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptOpenAlgorithmProvider(RNG) failed: 0x%08X", status);
            __leave;
        }

        status = BCryptGenRandom(hRng, pDerivedKey->rgbSalt, PBKDF2_SALT_SIZE, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptGenRandom failed: 0x%08X", status);
            __leave;
        }

        BCryptCloseAlgorithmProvider(hRng, 0);
        hRng = NULL;

        // Use manual PBKDF2-HMAC-SHA256 implementation
        // Password is passed as raw bytes (UTF-16)
        const BYTE* pbPassword = reinterpret_cast<const BYTE*>(pwszPassphrase);
        DWORD cbPassword = static_cast<DWORD>(cchPassphrase * sizeof(WCHAR));

        EIDM_TRACE_VERBOSE(L"Deriving keys with PBKDF2-HMAC-SHA256 (password: %u bytes, iterations: %u)",
            cbPassword, PBKDF2_ITERATIONS);

        // Derive encryption key
        cryptoStatus = PBKDF2HMACSHA256(
            pbPassword, cbPassword,
            pDerivedKey->rgbSalt, PBKDF2_SALT_SIZE,
            PBKDF2_ITERATIONS,
            PBKDF2_KEY_SIZE,
            pDerivedKey->rgbKey);

        if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
        {
            EIDM_TRACE_ERROR(L"PBKDF2 encryption key derivation failed");
            __leave;
        }

        // Derive auth key with modified salt
        BYTE rgbAuthSalt[PBKDF2_SALT_SIZE];
        memcpy(rgbAuthSalt, pDerivedKey->rgbSalt, PBKDF2_SALT_SIZE);
        for (DWORD i = 0; i < PBKDF2_SALT_SIZE; i++)
            rgbAuthSalt[i] ^= 0xFF;

        cryptoStatus = PBKDF2HMACSHA256(
            pbPassword, cbPassword,
            rgbAuthSalt, PBKDF2_SALT_SIZE,
            PBKDF2_ITERATIONS,
            PBKDF2_KEY_SIZE,
            pDerivedKey->rgbAuthKey);

        if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
        {
            EIDM_TRACE_ERROR(L"PBKDF2 auth key derivation failed");
            __leave;
        }

        status = STATUS_SUCCESS;
    }
    __finally
    {
        if (hRng) BCryptCloseAlgorithmProvider(hRng, 0);

        if (!BCRYPT_SUCCESS(status) && pDerivedKey)
            SecureZeroMemory(pDerivedKey, sizeof(DERIVED_KEY));
    }

    if (BCRYPT_SUCCESS(status))
        return CRYPTO_STATUS::CRYPTO_SUCCESS;
    else
        return CRYPTO_STATUS::CRYPTO_ERROR_KDF_FAILED;
}

// Derive keys from passphrase with provided salt (for decryption)
CRYPTO_STATUS DeriveKeyFromPassphraseWithSalt(
    _In_ PCWSTR pwszPassphrase,
    _In_ SIZE_T cchPassphrase,
    _In_reads_(PBKDF2_SALT_SIZE) const BYTE* pbSalt,
    _Out_ DERIVED_KEY* pDerivedKey)
{
    if (!pwszPassphrase || cchPassphrase == 0 || !pbSalt || !pDerivedKey)
    {
        return CRYPTO_STATUS::CRYPTO_ERROR_INVALID_PARAMETER;
    }

    CRYPTO_STATUS cryptoStatus = CRYPTO_STATUS::CRYPTO_SUCCESS;

    // Copy salt to output structure
    memcpy(pDerivedKey->rgbSalt, pbSalt, PBKDF2_SALT_SIZE);

    // Use manual PBKDF2-HMAC-SHA256 implementation
    const BYTE* pbPassword = reinterpret_cast<const BYTE*>(pwszPassphrase);
    DWORD cbPassword = static_cast<DWORD>(cchPassphrase * sizeof(WCHAR));

    EIDM_TRACE_VERBOSE(L"Deriving keys with PBKDF2-HMAC-SHA256 using provided salt (password: %u bytes, iterations: %u)",
        cbPassword, PBKDF2_ITERATIONS);

    // Derive encryption key
    cryptoStatus = PBKDF2HMACSHA256(
        pbPassword, cbPassword,
        pDerivedKey->rgbSalt, PBKDF2_SALT_SIZE,
        PBKDF2_ITERATIONS,
        PBKDF2_KEY_SIZE,
        pDerivedKey->rgbKey);

    if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
    {
        EIDM_TRACE_ERROR(L"PBKDF2 encryption key derivation failed");
        SecureZeroMemory(pDerivedKey, sizeof(DERIVED_KEY));
        return cryptoStatus;
    }

    // Derive auth key with modified salt
    BYTE rgbAuthSalt[PBKDF2_SALT_SIZE];
    memcpy(rgbAuthSalt, pDerivedKey->rgbSalt, PBKDF2_SALT_SIZE);
    for (DWORD i = 0; i < PBKDF2_SALT_SIZE; i++)
        rgbAuthSalt[i] ^= 0xFF;

    cryptoStatus = PBKDF2HMACSHA256(
        pbPassword, cbPassword,
        rgbAuthSalt, PBKDF2_SALT_SIZE,
        PBKDF2_ITERATIONS,
        PBKDF2_KEY_SIZE,
        pDerivedKey->rgbAuthKey);

    if (cryptoStatus != CRYPTO_STATUS::CRYPTO_SUCCESS)
    {
        EIDM_TRACE_ERROR(L"PBKDF2 auth key derivation failed");
        SecureZeroMemory(pDerivedKey, sizeof(DERIVED_KEY));
        return cryptoStatus;
    }

    return CRYPTO_STATUS::CRYPTO_SUCCESS;
}

BOOL ValidatePassphraseStrength(_In_ PCWSTR pwszPassphrase)
{
    if (!pwszPassphrase)
        return FALSE;

    size_t cchLen = wcslen(pwszPassphrase); // NOSONAR - pointer validated for NULL above (line 402)
    if (cchLen < 16)
        return FALSE;

    return TRUE;
}

CRYPTO_STATUS EncryptWithGCM(
    _In_ const BYTE* pbKey,
    _In_ DWORD cbKey,
    _In_reads_bytes_(cbNonce) const BYTE* pbNonce,
    _In_ DWORD cbNonce,
    _In_reads_bytes_(cbPlaintext) const BYTE* pbPlaintext,
    _In_ DWORD cbPlaintext,
    _Out_writes_(cbCiphertext) BYTE* pbCiphertext,
    _Inout_ DWORD* pcbCiphertext,
    _Out_writes_(cbTag) BYTE* pbTag,
    _In_ DWORD cbTag)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    DWORD cbResult = 0;

    __try
    {
        // Open AES-GCM algorithm provider
        status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptOpenAlgorithmProvider(AES) failed: 0x%08X", status);
            __leave;
        }

        // Set chaining mode to GCM
        status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE,
            (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0); // NOSONAR - Windows API requires non-const pointer, won't modify data
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptSetProperty(GCM) failed: 0x%08X", status);
            __leave;
        }

        // Import key
        status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, nullptr, 0,
            const_cast<PUCHAR>(pbKey), cbKey, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptGenerateSymmetricKey failed: 0x%08X", status);
            __leave;
        }

        // Setup auth info for GCM
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo = {};
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(pbNonce);
        authInfo.cbNonce = cbNonce;
        authInfo.pbTag = pbTag;
        authInfo.cbTag = cbTag;
        // No additional authenticated data

        EIDM_TRACE_VERBOSE(L"GCM encrypt: %u bytes, nonce=%u, tag=%u", cbPlaintext, cbNonce, cbTag);

        // Encrypt with GCM
        DWORD cbCiphertextResult = 0;
        status = BCryptEncrypt(hKey,
            const_cast<PUCHAR>(pbPlaintext), cbPlaintext,
            &authInfo,
            nullptr, 0,
            pbCiphertext, *pcbCiphertext,
            &cbCiphertextResult, 0);

        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptEncrypt failed: 0x%08X", status);
            __leave;
        }

        *pcbCiphertext = cbCiphertextResult;
    }
    __finally
    {
        if (hKey) BCryptDestroyKey(hKey);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
    }

    if (BCRYPT_SUCCESS(status))
        return CRYPTO_STATUS::CRYPTO_SUCCESS;
    else
        return (CRYPTO_STATUS)6;  // ERROR_ENCRYPTION_FAILED
}

CRYPTO_STATUS DecryptWithGCM(
    _In_ const BYTE* pbKey,
    _In_ DWORD cbKey,
    _In_reads_bytes_(cbNonce) const BYTE* pbNonce,
    _In_ DWORD cbNonce,
    _In_reads_bytes_(cbCiphertext) const BYTE* pbCiphertext,
    _In_ DWORD cbCiphertext,
    _In_reads_bytes_(cbTag) const BYTE* pbTag,
    _In_ DWORD cbTag,
    _Out_writes_(cbPlaintext) BYTE* pbPlaintext,
    _Inout_ DWORD* pcbPlaintext)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    BCRYPT_KEY_HANDLE hKey = NULL;
    DWORD cbResult = 0;

    __try
    {
        // Open AES-GCM algorithm provider
        status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_AES_ALGORITHM, nullptr, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptOpenAlgorithmProvider(AES) failed: 0x%08X", status);
            __leave;
        }

        // Set chaining mode to GCM
        status = BCryptSetProperty(hAlgorithm, BCRYPT_CHAINING_MODE,
            (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0); // NOSONAR - Windows API requires non-const pointer, won't modify data
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptSetProperty(GCM) failed: 0x%08X", status);
            __leave;
        }

        // Import key
        status = BCryptGenerateSymmetricKey(hAlgorithm, &hKey, nullptr, 0,
            const_cast<PUCHAR>(pbKey), cbKey, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptGenerateSymmetricKey failed: 0x%08X", status);
            __leave;
        }

        // Setup auth info for GCM
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo = {};
        BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
        authInfo.pbNonce = const_cast<PUCHAR>(pbNonce);
        authInfo.cbNonce = cbNonce;
        authInfo.pbTag = const_cast<PUCHAR>(pbTag);
        authInfo.cbTag = cbTag;

        EIDM_TRACE_VERBOSE(L"GCM decrypt: %u bytes, nonce=%u, tag=%u", cbCiphertext, cbNonce, cbTag);

        // Decrypt with GCM (tag verification happens automatically)
        status = BCryptDecrypt(hKey,
            const_cast<PUCHAR>(pbCiphertext), cbCiphertext,
            &authInfo,
            nullptr, 0,
            pbPlaintext, *pcbPlaintext,
            &cbResult, 0);

        if (!BCRYPT_SUCCESS(status))
        {
            EIDM_TRACE_ERROR(L"BCryptDecrypt failed: 0x%08X", status);
            __leave;
        }

        *pcbPlaintext = cbResult;
    }
    __finally
    {
        if (hKey) BCryptDestroyKey(hKey);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
    }

    if (BCRYPT_SUCCESS(status))
        return CRYPTO_STATUS::CRYPTO_SUCCESS;
    else
        return (CRYPTO_STATUS)7;  // ERROR_DECRYPTION_FAILED
}

std::string EncodeBase64(_In_reads_bytes_(cbData) const BYTE* pbData, _In_ DWORD cbData)
{
    if (!pbData || cbData == 0)
        return std::string();

    DWORD dwLength = 0;
    if (!CryptBinaryToStringA(pbData, cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        nullptr, &dwLength))
        return std::string();

    std::string result(dwLength, 0);
    if (!CryptBinaryToStringA(pbData, cbData, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF,
        &result[0], &dwLength))
        return std::string();

    // Remove null terminator
    result.pop_back();
    return result;
}

std::vector<BYTE> DecodeBase64(_In_ const std::string& encoded)
{
    if (encoded.empty())
        return std::vector<BYTE>();

    DWORD dwLength = 0;
    if (!CryptStringToBinaryA(encoded.c_str(), static_cast<DWORD>(encoded.length()),
        CRYPT_STRING_BASE64, nullptr, &dwLength, nullptr, nullptr))
        return std::vector<BYTE>();

    std::vector<BYTE> result(dwLength);
    if (!CryptStringToBinaryA(encoded.c_str(), static_cast<DWORD>(encoded.length()),
        CRYPT_STRING_BASE64, result.data(), &dwLength, nullptr, nullptr))
        return std::vector<BYTE>();

    return result;
}

BOOL ComputeHMAC(
    _In_reads_bytes_(cbKey) const BYTE* pbKey,
    _In_ DWORD cbKey,
    _In_reads_bytes_(cbData) const BYTE* pbData,
    _In_ DWORD cbData,
    _Out_writes_all_(HASH_SIZE) BYTE* pbHash,
    _In_ DWORD HASH_SIZE)
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    PBYTE pbHashObject = nullptr;
    DWORD cbHashObject = 0;
    DWORD cbResult = 0;

    __try
    {
        // Open HMAC algorithm provider
        status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_HASH_INTERFACE_HMAC);
        if (!BCRYPT_SUCCESS(status))
        {
            __leave;
        }

        // Get hash object size
        status = BCryptGetProperty(hAlgorithm, BCRYPT_OBJECT_LENGTH,
            (PBYTE)&cbHashObject, sizeof(DWORD), &cbResult, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            __leave;
        }

        pbHashObject = static_cast<PBYTE>(malloc(cbHashObject)); // NOSONAR - BCrypt API requires malloc/free for hash object buffer
        if (!pbHashObject)
        {
            status = STATUS_NO_MEMORY;
            __leave;
        }

        // Create hash handle
        status = BCryptCreateHash(hAlgorithm, &hHash, pbHashObject, cbHashObject,
            reinterpret_cast<PBYTE>(const_cast<BYTE*>(pbKey)), cbKey, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            __leave;
        }

        // Hash data
        status = BCryptHashData(hHash, const_cast<PBYTE>(pbData), cbData, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            __leave;
        }

        // Get hash result
        status = BCryptFinishHash(hHash, pbHash, HASH_SIZE, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            __leave;
        }
    }
    __finally
    {
        if (pbHashObject) free(pbHashObject); // NOSONAR - memory allocated with malloc for BCrypt API compatibility
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlgorithm) BCryptCloseAlgorithmProvider(hAlgorithm, 0);
    }

    return BCRYPT_SUCCESS(status);
}

BOOL GenerateRandom(_Out_writes_all_(cbBytes) BYTE* pbBytes, _In_ DWORD cbBytes)
{
    BCRYPT_ALG_HANDLE hAlgorithm = NULL;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlgorithm, BCRYPT_RNG_ALGORITHM, nullptr, 0);

    if (BCRYPT_SUCCESS(status))
    {
        status = BCryptGenRandom(hAlgorithm, pbBytes, cbBytes, 0);
        BCryptCloseAlgorithmProvider(hAlgorithm, 0);
    }

    return BCRYPT_SUCCESS(status);
}

std::string BytesToHex(_In_reads_bytes_(cbBytes) const BYTE* pbBytes, _In_ DWORD cbBytes)
{
    std::string result;
    result.reserve(cbBytes * 2);

    for (DWORD i = 0; i < cbBytes; i++)
    {
        char szHex[3];
        sprintf_s(szHex, "%02x", pbBytes[i]);
        result += szHex;
    }

    return result;
}

std::vector<BYTE> HexToBytes(_In_ const std::string& hex)
{
    std::vector<BYTE> result;

    if (hex.length() % 2 != 0)
        return result;

    for (size_t i = 0; i < hex.length(); i += 2)
    {
        BYTE b = 0;
        for (int j = 0; j < 2; j++)
        {
            char c = hex[i + j];
            b <<= 4;

            if (c >= '0' && c <= '9')
                b |= (c - '0');
            else if (c >= 'a' && c <= 'f')
                b |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                b |= (c - 'A' + 10);
        }

        result.push_back(b);
    }

    return result;
}
