
#include <windows.h>
#include <tchar.h>
#include "../EIDCardLibrary/GPO.h"

void Menu_AP_GPO()
{
	WCHAR buffer[4096];
	
	LPWSTR pMessage = L"AllowSignatureOnlyKeys: %d\n"
	L"AllowCertificatesWithNoEKU %d\n"
	L"AllowTimeInvalidCertificates %d\n"
	L"AllowIntegratedUnblock %d\n"
	L"ReverseSubject %d\n"
	L"X509HintsNeeded %d\n"
	L"IntegratedUnblockPromptString %d\n"
	L"CertPropEnabledString %d\n"
	L"CertPropRootEnabledString %d\n"
	L"RootsCleanupOption %d\n"
	L"FilterDuplicateCertificates %d\n"
	L"ForceReadingAllCertificates %d\n"
	L"scforceoption %d\n"
	L"scremoveoption %d";

	swprintf_s(buffer,4096,pMessage,GetPolicyValue(AllowSignatureOnlyKeys),
		GetPolicyValue(AllowCertificatesWithNoEKU),
		GetPolicyValue(AllowTimeInvalidCertificates),
		GetPolicyValue(AllowIntegratedUnblock),
		GetPolicyValue(ReverseSubject),
		GetPolicyValue(X509HintsNeeded),
		GetPolicyValue(IntegratedUnblockPromptString),
		GetPolicyValue(CertPropEnabledString),
		GetPolicyValue(CertPropRootEnabledString),
		GetPolicyValue(RootsCleanupOption),
		GetPolicyValue(FilterDuplicateCertificates),
		GetPolicyValue(ForceReadingAllCertificates),
		GetPolicyValue(scforceoption),
		GetPolicyValue(scremoveoption));
	MessageBox(NULL,buffer,L"Policy",0);

}