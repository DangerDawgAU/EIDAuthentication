/*
    EID Authentication - fuzz/regression harness
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
// Deterministic replay of the proof-of-concept inputs for the four defects
// found on 2026-07-30. Fast, no fuzzing, runs on every push.
//
// SCOPE - be precise about what this proves:
//   Each case asserts that the CANONICAL VALIDATOR rejects an input that the
//   pre-fix code accepted. That protects against a future change weakening a
//   validator, and it pins the exact byte patterns for the record.
//
//   It does NOT prove the production call sites invoke the validator - a
//   validator can be perfect and unused. That property is covered two ways:
//   the libFuzzer targets assert "validator says OK => the consumer's own
//   offset arithmetic stays in bounds", and the call-site wiring is reviewed
//   in the diff. See docs/FUZZING.md.
//
// Exit code = number of failing cases, so CI can gate on it.
//=============================================================================

#include <windows.h>
#include <stdio.h>
#include <vector>

#include "EIDCardLibrary.h"
#include "StoredCredentialManagement.h"
#include "InputValidation.h"

namespace {

int g_failures = 0;
int g_total = 0;

void Check(const char* pszCase, bool fRejected, const char* pszWhy)
{
	g_total++;
	if (fRejected)
	{
		printf("  PASS  %-46s (rejected)\n", pszCase);
	}
	else
	{
		printf("  FAIL  %-46s ACCEPTED - %s\n", pszCase, pszWhy);
		g_failures++;
	}
}

void CheckAccepted(const char* pszCase, bool fAccepted)
{
	g_total++;
	if (fAccepted)
	{
		printf("  PASS  %-46s (accepted)\n", pszCase);
	}
	else
	{
		printf("  FAIL  %-46s REJECTED - benign input must still work\n", pszCase);
		g_failures++;
	}
}

//-------------------------------------------------------------------------
// Defect 1 - SSP token messages parsed with no length validation at all.
// CredentialManagement.cpp:443 / :495 before the fix.
//-------------------------------------------------------------------------
void TestTokenMessages()
{
	printf("\nDefect 1: SSP token messages (CredentialManagement.cpp:443,:495)\n");

	// 1a. UsernameLen = 0xFFFFFFFF. The consumer did
	//     EIDAlloc(UsernameLen + sizeof(WCHAR)) -> wraps to EIDAlloc(1),
	//     then memcpy'd 4 GB into it. Heap overflow inside LSASS.
	{
		std::vector<BYTE> token(sizeof(EID_CHALLENGE_MESSAGE) + 64, 0);
		auto* p = reinterpret_cast<EID_CHALLENGE_MESSAGE*>(token.data());
		memcpy(p->Signature.data(), EID_MESSAGE_SIGNATURE, p->Signature.size());
		p->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTChallenge);
		p->Version = EID_MESSAGE_VERSION;
		p->UsernameLen = 0xFFFFFFFF;
		p->UsernameOffset = sizeof(EID_CHALLENGE_MESSAGE);
		p->ChallengeLen = 16;
		p->ChallengeOffset = sizeof(EID_CHALLENGE_MESSAGE);
		Check("challenge: UsernameLen=0xFFFFFFFF wrap",
			!EIDValidateChallengeMessage(token.data(), static_cast<DWORD>(token.size())),
			"terminator allowance wraps DWORD");
	}

	// 1b. Offset far past the end of the token, used as a raw pointer
	//     displacement -> arbitrary OOB read relative to the LSASS heap.
	{
		std::vector<BYTE> token(sizeof(EID_CHALLENGE_MESSAGE) + 64, 0);
		auto* p = reinterpret_cast<EID_CHALLENGE_MESSAGE*>(token.data());
		memcpy(p->Signature.data(), EID_MESSAGE_SIGNATURE, p->Signature.size());
		p->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTChallenge);
		p->Version = EID_MESSAGE_VERSION;
		p->UsernameLen = 4;
		p->UsernameOffset = sizeof(EID_CHALLENGE_MESSAGE);
		p->ChallengeLen = 0x100;
		p->ChallengeOffset = 0xFFFFFF00;
		Check("challenge: ChallengeOffset past end",
			!EIDValidateChallengeMessage(token.data(), static_cast<DWORD>(token.size())),
			"offset outside token");
	}

	// 1c. Token shorter than the fixed header. The pre-fix code dereferenced
	//     MessageType before establishing any size at all.
	{
		std::vector<BYTE> token(8, 0);
		Check("challenge: token smaller than header",
			!EIDValidateChallengeMessage(token.data(), static_cast<DWORD>(token.size())),
			"header read would be OOB");
	}

	// 1d. Response message, same offset defect.
	{
		std::vector<BYTE> token(sizeof(EID_RESPONSE_MESSAGE) + 32, 0);
		auto* p = reinterpret_cast<EID_RESPONSE_MESSAGE*>(token.data());
		memcpy(p->Signature.data(), EID_MESSAGE_SIGNATURE, p->Signature.size());
		p->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTResponse);
		p->Version = EID_MESSAGE_VERSION;
		p->ResponseLen = 0xFFFFFFF0;
		p->ResponseOffset = sizeof(EID_RESPONSE_MESSAGE);
		Check("response: ResponseLen exceeds token",
			!EIDValidateResponseMessage(token.data(), static_cast<DWORD>(token.size())),
			"length outside token");
	}

	// 1e. A well-formed token must still be accepted, or we have merely
	//     broken authentication instead of fixing it.
	{
		const DWORD cbPayload = 32;
		std::vector<BYTE> token(sizeof(EID_CHALLENGE_MESSAGE) + cbPayload, 0);
		auto* p = reinterpret_cast<EID_CHALLENGE_MESSAGE*>(token.data());
		memcpy(p->Signature.data(), EID_MESSAGE_SIGNATURE, p->Signature.size());
		p->MessageType = static_cast<DWORD>(EID_MESSAGE_TYPE::EIDMTChallenge);
		p->Version = EID_MESSAGE_VERSION;
		p->ChallengeOffset = sizeof(EID_CHALLENGE_MESSAGE);
		p->ChallengeLen = 16;
		p->UsernameOffset = sizeof(EID_CHALLENGE_MESSAGE) + 16;
		p->UsernameLen = 16;
		CheckAccepted("challenge: well-formed token still valid",
			EIDValidateChallengeMessage(token.data(), static_cast<DWORD>(token.size())) != FALSE);
	}
}

//-------------------------------------------------------------------------
// Defects 2 and 3 - EID_SMARTCARD_CSP_INFO.
// dwCspInfoLen was never bounded against the length the LSA supplied, and
// the debug printer indexed bBuffer with a raw 32-bit offset.
//-------------------------------------------------------------------------
void TestCspInfo()
{
	printf("\nDefects 2+3: CSP info (Package.cpp:553,:628)\n");

	// Helper: allocate a CSP_INFO of a given real size.
	auto MakeCspInfo = [](DWORD cbReal) {
		std::vector<BYTE> buf(cbReal, 0);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		p->dwCspInfoLen = cbReal;
		return buf;
	};

	// 3a. THE defect: a 40-byte buffer declaring a 64 KB interior. Both
	//     downstream validators bounded their offsets against dwCspInfoLen,
	//     so this passed every check and read ~64 KB out of bounds.
	{
		auto buf = MakeCspInfo(40);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		p->dwCspInfoLen = 0x10000;
		p->nCardNameOffset = 0xFFF0;
		Check("cspinfo: dwCspInfoLen exceeds CspDataLength",
			!EIDValidateCspInfo(p, 40),
			"inner length is attacker data, must be bounded");
	}

	// 3b. Offset chosen so that offset * sizeof(WCHAR) overflows 32 bits.
	{
		auto buf = MakeCspInfo(128);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		p->nReaderNameOffset = 0xFFFFFFF0;
		Check("cspinfo: char offset multiply overflows",
			!EIDValidateCspInfo(p, 128),
			"offset scaling must be overflow-checked");
	}

	// 3c. Offset inside bounds but the string is never NUL-terminated, so
	//     _tcscmp / CryptAcquireContext walk past the end.
	{
		auto buf = MakeCspInfo(64);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		const DWORD hdr = static_cast<DWORD>(FIELD_OFFSET(EID_SMARTCARD_CSP_INFO, bBuffer));
		// Fill the whole tail with non-NUL WCHARs.
		for (DWORD off = hdr; off + 1 < 64; off += 2)
		{
			buf[off] = 0x41;
			buf[off + 1] = 0x00;
		}
		p->nContainerNameOffset = 1;
		Check("cspinfo: unterminated name string",
			!EIDValidateCspInfo(p, 64),
			"string must terminate inside the buffer");
	}

	// 3d. Accessor must refuse rather than hand back a pointer to re-check.
	{
		auto buf = MakeCspInfo(40);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		p->dwCspInfoLen = 0x10000;
		Check("cspinfo: accessor refuses invalid struct",
			EIDCspInfoStringAt(p, 40, 0xFFF0) == nullptr,
			"accessor returned a pointer into OOB memory");
	}

	// 3e. A benign CSP info with one real, terminated name must work.
	{
		const DWORD hdr = static_cast<DWORD>(FIELD_OFFSET(EID_SMARTCARD_CSP_INFO, bBuffer));
		const DWORD cb = hdr + 8 * sizeof(WCHAR);
		auto buf = MakeCspInfo(cb);
		auto* p = reinterpret_cast<EID_SMARTCARD_CSP_INFO*>(buf.data());
		WCHAR* pName = reinterpret_cast<WCHAR*>(buf.data() + hdr);
		pName[0] = L'C'; pName[1] = L'a'; pName[2] = L'r'; pName[3] = L'd'; pName[4] = L'\0';
		p->nCardNameOffset = 0;   // 0 means absent, so use offset 1 instead
		pName[5] = L'\0';
		p->nContainerNameOffset = 1;
		CheckAccepted("cspinfo: benign struct still valid",
			EIDValidateCspInfo(p, cb) != FALSE);
	}
}

//-------------------------------------------------------------------------
// Defect 4 - EID_PRIVATE_DATA blob.
// usPasswordLen == 0 underflowed dwRoundNumber; overlapping regions made the
// SecureZeroMemory span exceed the allocation.
//-------------------------------------------------------------------------
void TestPrivateData()
{
	printf("\nDefect 4: private-data blob (StoredCredentialManagement.cpp:77,:1751,:1768)\n");

	const DWORD hdr = static_cast<DWORD>(FIELD_OFFSET(EID_PRIVATE_DATA, Data));

	// 4a. usPasswordLen = 0 -> dwRoundNumber = 0 -> (dwRoundNumber - 1)
	//     underflows -> terminator written ~4 GB past a 2-byte allocation.
	{
		std::vector<BYTE> blob(hdr + 64, 0);
		auto* p = reinterpret_cast<EID_PRIVATE_DATA*>(blob.data());
		p->dwCertificatOffset = 0;
		p->dwCertificatSize = 16;
		p->dwSymetricKeyOffset = 16;
		p->dwSymetricKeySize = 16;
		p->dwPasswordOffset = 32;
		p->usPasswordLen = 0;
		Check("blob: usPasswordLen=0 underflow",
			!EIDValidatePrivateDataLayout(p, static_cast<DWORD>(blob.size())),
			"zero length underflows dwRoundNumber");
	}

	// 4b. Three regions overlapping at offset 0. Each fits individually, so
	//     the old per-region check passed; summing their sizes for
	//     SecureZeroMemory then overran the allocation.
	{
		const DWORD cbData = 32;
		std::vector<BYTE> blob(hdr + cbData, 0);
		auto* p = reinterpret_cast<EID_PRIVATE_DATA*>(blob.data());
		p->dwCertificatOffset = 0;  p->dwCertificatSize = static_cast<USHORT>(cbData);
		p->dwSymetricKeyOffset = 0; p->dwSymetricKeySize = static_cast<USHORT>(cbData);
		p->dwPasswordOffset = 0;    p->usPasswordLen = static_cast<USHORT>(cbData);
		Check("blob: overlapping regions inflate zeroize span",
			!EIDValidatePrivateDataLayout(p, static_cast<DWORD>(blob.size())),
			"sum of sizes exceeds allocation");
	}

	// 4c. Region extending one byte past the data area.
	{
		const DWORD cbData = 32;
		std::vector<BYTE> blob(hdr + cbData, 0);
		auto* p = reinterpret_cast<EID_PRIVATE_DATA*>(blob.data());
		p->dwCertificatOffset = 0;   p->dwCertificatSize = 16;
		p->dwSymetricKeyOffset = 16; p->dwSymetricKeySize = 8;
		p->dwPasswordOffset = 24;    p->usPasswordLen = 9;   // 24+9 = 33 > 32
		Check("blob: region overruns data area by one",
			!EIDValidatePrivateDataLayout(p, static_cast<DWORD>(blob.size())),
			"off-by-one at the region boundary");
	}

	// 4d. Blob smaller than the header.
	{
		std::vector<BYTE> blob(4, 0);
		Check("blob: smaller than header",
			!EIDValidatePrivateDataLayout(reinterpret_cast<EID_PRIVATE_DATA*>(blob.data()), 4),
			"header read would be OOB");
	}

	// 4e. Well-formed blob accepted, and its zeroize span must be inside the
	//     allocation - the property the fix depends on.
	{
		const DWORD cbData = 48;
		std::vector<BYTE> blob(hdr + cbData, 0);
		auto* p = reinterpret_cast<EID_PRIVATE_DATA*>(blob.data());
		p->dwCertificatOffset = 0;   p->dwCertificatSize = 16;
		p->dwSymetricKeyOffset = 16; p->dwSymetricKeySize = 16;
		p->dwPasswordOffset = 32;    p->usPasswordLen = 16;
		const DWORD cbBlob = static_cast<DWORD>(blob.size());
		CheckAccepted("blob: benign layout still valid",
			EIDValidatePrivateDataLayout(p, cbBlob) != FALSE);

		const DWORD dwSpan = EIDPrivateDataSpan(p, cbBlob);
		g_total++;
		if (dwSpan > 0 && dwSpan <= cbBlob)
		{
			printf("  PASS  %-46s (span %u <= %u)\n", "blob: zeroize span within allocation", dwSpan, cbBlob);
		}
		else
		{
			printf("  FAIL  %-46s span %u vs allocation %u\n", "blob: zeroize span within allocation", dwSpan, cbBlob);
			g_failures++;
		}
	}
}

} // namespace

int main()
{
	printf("EID input-validation regression replay\n");
	printf("======================================\n");

	TestTokenMessages();
	TestCspInfo();
	TestPrivateData();

	printf("\n%d/%d checks passed.\n", g_total - g_failures, g_total);
	if (g_failures != 0)
	{
		printf("REGRESSION: %d check(s) failed.\n", g_failures);
	}
	return g_failures;
}
