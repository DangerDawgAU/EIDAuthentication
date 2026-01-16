/*
    EID Authentication - Smart card authentication for Windows
    Copyright (C) 2009 Vincent Le Toux
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


PCCERT_CONTEXT GetCertificateFromCspInfo(__in PEID_SMARTCARD_CSP_INFO pCspInfo);
BOOL IsTrustedCertificate(__in PCCERT_CONTEXT pCertContext, __in_opt DWORD dwFlag = 0);
BOOL HasCertificateRightEKU(__in PCCERT_CONTEXT pCertContext);
LPCTSTR GetTrustErrorText(DWORD Status);
BOOL MakeTrustedCertifcate(PCCERT_CONTEXT pCertContext);

// CSP provider whitelist validation - prevents malicious CSP injection
BOOL IsAllowedCSPProvider(__in LPCWSTR pwszProviderName);