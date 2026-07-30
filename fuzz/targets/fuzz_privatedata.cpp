/*
    EID Authentication - fuzz target: EID_PRIVATE_DATA stored-credential blob
    Copyright (C) 2026 Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//=============================================================================
// THE ORACLE - two properties, both of which failed in the pre-fix code:
//
//   1. EIDValidatePrivateDataLayout() == TRUE
//        =>  reading each declared region stays inside the blob.
//
//   2. EIDValidatePrivateDataLayout() == TRUE
//        =>  EIDPrivateDataSpan() <= the allocation size, so the cleanup
//            SecureZeroMemory cannot overrun.
//
// Property 2 is the interesting one. The old code zeroized
//     sizeof(EID_PRIVATE_DATA) + dwCertificatSize + dwSymetricKeySize
//                              + usPasswordLen
// while the validator only checked each region fit INDIVIDUALLY. Three
// overlapping regions therefore produced a span up to 3x the allocation. The
// zeroize is reproduced literally below, against an exact-size allocation, so
// any inflated span is an immediate ASan heap-buffer-overflow WRITE.
//
// The dwRoundNumber underflow (usPasswordLen == 0) is also modelled, since
// that is arithmetic the validator must make impossible rather than something
// the consumer can defend against locally.
//=============================================================================

#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "EIDCardLibrary.h"
#include "StoredCredentialManagement.h"
#include "InputValidation.h"
#include "oracle.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	const DWORD dwHeaderSize = static_cast<DWORD>(FIELD_OFFSET(EID_PRIVATE_DATA, Data));
	if (size < dwHeaderSize || size > (1u << 20))
	{
		return 0;
	}

	void* pRaw = _aligned_malloc(size, 8);
	if (!pRaw)
	{
		return 0;
	}
	memcpy(pRaw, data, size);

	const EID_PRIVATE_DATA* pBlob = static_cast<const EID_PRIVATE_DATA*>(pRaw);
	const DWORD cbBlob = static_cast<DWORD>(size);

	if (EIDValidatePrivateDataLayout(pBlob, cbBlob))
	{
		const BYTE* pData = reinterpret_cast<const BYTE*>(pRaw) + dwHeaderSize;

		// Property 1: read every region the way the consumers do
		// (CertCreateCertificateContext over the cert bytes, CryptImportKey
		// over the symmetric key, CryptDecrypt over the password).
		volatile BYTE bSink = 0;
		for (DWORD i = 0; i < pBlob->dwCertificatSize; i++)
		{
			bSink ^= pData[pBlob->dwCertificatOffset + i];
		}
		for (DWORD i = 0; i < pBlob->dwSymetricKeySize; i++)
		{
			bSink ^= pData[pBlob->dwSymetricKeyOffset + i];
		}
		for (DWORD i = 0; i < pBlob->usPasswordLen; i++)
		{
			bSink ^= pData[pBlob->dwPasswordOffset + i];
		}
		(void)bSink;

		// Model the AES decrypt loop's index arithmetic. A zero usPasswordLen
		// makes dwRoundNumber 0 and (dwRoundNumber - 1) underflow; the
		// validator must have excluded it.
		const DWORD dwBlockLen = 16;   // AES-128, as CryptGetKeyParam reports
		const DWORD dwRoundNumber = (pBlob->usPasswordLen / dwBlockLen) +
			((pBlob->usPasswordLen % dwBlockLen) ? 1 : 0);
		EID_ORACLE_REQUIRE(dwRoundNumber != 0,
			"usPasswordLen yields dwRoundNumber == 0, so (dwRoundNumber - 1) "
			"underflows and the terminator is written ~4 GB out of bounds");

		// Property 2: the literal cleanup write, against an exact-size buffer.
		const DWORD dwSpan = EIDPrivateDataSpan(pBlob, cbBlob);
		EID_ORACLE_REQUIRE(dwSpan != 0 && dwSpan <= cbBlob,
			"zeroize span is zero or exceeds the allocation - SecureZeroMemory "
			"in the cleanup path would write past the end of the blob");
		void* pScratch = malloc(cbBlob);
		if (pScratch)
		{
			memcpy(pScratch, pRaw, cbBlob);
			SecureZeroMemory(pScratch, dwSpan);
			free(pScratch);
		}
	}

	_aligned_free(pRaw);
	return 0;
}
