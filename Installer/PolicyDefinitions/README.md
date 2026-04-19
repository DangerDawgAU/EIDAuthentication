# EID Authentication - Group Policy Templates

This directory contains the ADMX / ADML files that expose the **custom**
EID Authentication policies in the Group Policy Editor (`gpedit.msc`) and
the domain Group Policy Management Console (GPMC).

## Files

- `EIDAuthentication.admx` - policy definitions
- `en-US/EIDAuthentication.adml` - English display strings

## Scope

Only policies introduced by this product are defined here. The smart-card
policies that the product also consumes (for example `AllowSignatureOnlyKeys`,
`AllowIntegratedUnblock`, `FilterDuplicateCertificates`, and so on) are
standard Windows policies already defined in the Microsoft-supplied
`SmartCard.admx` that ships with Windows, and can be configured through that
template. All policies are read from the same registry hive:

```
HKLM\SOFTWARE\Policies\Microsoft\Windows\SmartCardCredentialProvider
```

## Deployment

### Local machine
The NSIS installer copies these files to:

- `%WINDIR%\PolicyDefinitions\EIDAuthentication.admx`
- `%WINDIR%\PolicyDefinitions\en-US\EIDAuthentication.adml`

They then appear under:
`Computer Configuration \ Administrative Templates \ Windows Components \ EID Authentication`

### Domain (Group Policy Central Store)
For domain deployment, copy the files to the PolicyDefinitions central store
on a domain controller:

```
\\<domain>\SYSVOL\<domain>\Policies\PolicyDefinitions\EIDAuthentication.admx
\\<domain>\SYSVOL\<domain>\Policies\PolicyDefinitions\en-US\EIDAuthentication.adml
```

Once replicated, every GPMC console in the domain picks up the template
automatically - no per-admin-workstation install is required.

## Policies defined

| Policy | Registry value | Default |
|--------|---------------|---------|
| Enforce CSP whitelist | `EnforceCSPWhitelist` (DWORD) | `0` (Disabled) |

`EnforceCSPWhitelist` is the security-critical addition - when enabled it
blocks smart-card certificates that use a CSP / KSP provider outside the
built-in whitelist, mitigating attacker-controlled cryptographic provider
substitution.
