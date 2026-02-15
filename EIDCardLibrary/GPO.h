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


#pragma once

#include <utility>

enum GPOPolicy
{
  AllowSignatureOnlyKeys,
  AllowCertificatesWithNoEKU,
  AllowTimeInvalidCertificates,
  AllowIntegratedUnblock,
  ReverseSubject,
  X509HintsNeeded,
  IntegratedUnblockPromptString,
  CertPropEnabledString,
  CertPropRootEnabledString,
  RootsCleanupOption,
  FilterDuplicateCertificates,
  ForceReadingAllCertificates,
  scforceoption,
  scremoveoption,
  EnforceCSPWhitelist,  // Security: block CSP providers not in whitelist
};

// Validates that a GPOPolicy enum value is within valid bounds to prevent array overflow
// Marked constexpr+noexcept for compile-time evaluation and LSASS compatibility
constexpr bool IsValidPolicy(GPOPolicy policy) noexcept
{
    return policy >= AllowSignatureOnlyKeys && policy <= EnforceCSPWhitelist;
}

// Compile-time validation of GPOPolicy enum bounds
static_assert(IsValidPolicy(AllowSignatureOnlyKeys), "AllowSignatureOnlyKeys must be a valid policy");
static_assert(IsValidPolicy(EnforceCSPWhitelist), "EnforceCSPWhitelist must be a valid policy");

DWORD GetPolicyValue(GPOPolicy Policy);
BOOL SetPolicyValue(GPOPolicy Policy, DWORD dwValue);
