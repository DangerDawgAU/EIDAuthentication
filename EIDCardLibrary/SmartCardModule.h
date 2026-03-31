
#pragma once

// Note: PEID_SMARTCARD_CSP_INFO is defined in EIDCardLibrary.h
// cardmod.h is available in the repository's include directory

BOOL CheckPINandGetRemainingAttempts(PTSTR szReader, PTSTR szCard, PTSTR szPin, PDWORD pdwAttempts);
NTSTATUS CheckPINandGetRemainingAttemptsIfPossible(PEID_SMARTCARD_CSP_INFO pCspInfo, PTSTR szPin, NTSTATUS *pSubStatus);

