# EIDLogManager Flexible Logging Requirements Plan

## Document Information

| Field | Value |
|-------|-------|
| **Date** | 2026-03-26 |
| **Version** | 1.0 |
| **Status** | Draft |
| **Purpose** | Define requirements for flexible, event-based CSV logging |

---

## Executive Summary

The current EIDLogManager uses a monolithic log level approach (Error/Warning/Info/Verbose) that writes to Windows ETW (Event Tracing for Windows) binary files. This document outlines requirements for a flexible, event-based logging system that outputs to CSV format with configurable columns and event-specific filtering.

### Key Goals

1. **Event-Selective Logging** - Enable specific events without enabling entire log levels
2. **Flexible CSV Output** - Configurable column selection for different use cases
3. **Industry Compliance** - Align with OWASP and Syslog (RFC 5424) standards
4. **Backward Compatibility** - Maintain existing ETW logging alongside new CSV output

---

## Current State Analysis

### Existing Implementation

| Component | Description |
|-----------|-------------|
| **Trace Format** | Windows ETW binary (.etl files) |
| **Levels** | Error (2), Warning (3), Info (4), Verbose (5) |
| **Configuration** | Registry-based; stored at `HKLM\SOFTWARE\EIDAuthentication` |
| **Export** | Converts ETL to text via `ExportOneTraceFile()` |
| **File Rotation** | Configurable max size and file count |
| **GUI** | EIDLogManager.exe for configuration |

### Limitations

1. **Coarse-Grained Control** - Cannot selectively log specific event types
2. **Binary Format** - ETL requires conversion for analysis
3. **Fixed Fields** - No flexibility in what information is captured
4. **No Event Categorization** - All events grouped by severity only

---

## Industry Best Practices Research

### OWASP Logging Cheat Sheet Key Recommendations

The OWASP Logging Cheat Sheet identifies critical event attributes that must be logged:

| Attribute | Description | Priority |
|-----------|-------------|----------|
| **When** | Log date/time in international format (ISO 8601) | Required |
| **Where** | Application identifier, hostname, IP address | Required |
| **Who** | Source address, user identity (if authenticated) | Required |
| **What** | Event type, severity, security-relevant flag | Required |
| **Action** | Original intended purpose (Log in, Log out, etc.) | Recommended |
| **Object** | Affected component (user account, file, resource) | Recommended |
| **Result** | Success/Fail/Defer status | Recommended |
| **Reason** | Why the status occurred | Recommended |

**Events That Must Be Logged** (per OWASP):
- Authentication successes and failures
- Authorization (access control) failures
- Session management failures
- Input validation failures
- Use of higher-risk functionality (user admin, privilege use, sensitive data access)
- Configuration changes
- Application start-ups and shut-downs

### RFC 5424 Syslog Protocol Standard

The Syslog standard defines structured message elements:

| Field | Format | Description |
|-------|--------|-------------|
| **PRI** | `< number >` | Priority = (Facility × 8) + Severity |
| **VERSION** | Digit | Protocol version (currently "1") |
| **TIMESTAMP** | ISO 8601 | `YYYY-MM-DDThh:mm:ss.sss+/-hh:mm` |
| **HOSTNAME** | FQDN or IP | Machine that sent the message |
| **APP-NAME** | String | Application identifier |
| **PROCID** | String | Process ID (for grouping) |
| **MSGID** | String | Message type identifier |
| **STRUCTURED-DATA** | `[SD-ID PARAM="value"]` | Key-value pairs |
| **MSG** | UTF-8 | Free-form message |

**Severity Levels** (0-7):
- 0 = Emergency, 1 = Alert, 2 = Critical
- 3 = Error, 4 = Warning, 5 = Notice
- 6 = Informational, 7 = Debug

**Sources:**
- [OWASP Logging Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html)
- [RFC 5424: The Syslog Protocol](https://www.rfc-editor.org/rfc/rfc5424)

---

## Functional Requirements

### REQ-1: Event Categorization

The system shall categorize all security-relevant events into distinct categories:

| Category | Prefix | Description | Example Events |
|----------|--------|-------------|----------------|
| **AUTHENTICATION** | EID-1xxx | Login attempts, credential validation | Smart card detected, PIN verify, certificate validate |
| **AUTHORIZATION** | EID-2xxx | Permission checks, access grants | LSA secret read/write, credential store access |
| **SESSION** | EID-3xxx | Session lifecycle | Logon, logoff, session renew, lock/unlock |
| **CERTIFICATE** | EID-4xxx | Certificate operations | Cert chain validation, expiry check, cert install |
| **SMARTCARD** | EID-5xxx | Card reader interactions | Card insert/eject, ATR read, reader connect |
| **LSA** | EID-6xxx | LSA secret operations | Secret create/read/delete, secret name format |
| **CONFIG** | EID-7xxx | Configuration changes | Registry changes, log settings modified |
| **AUDIT** | EID-8xxx | High-risk security events | Admin actions, credential export/import |

### REQ-2: Configurable Event Filtering

The system shall support enabling/dislogging events at multiple granularities:

1. **Category-level** - Enable all events in a category
2. **Event-level** - Enable specific event IDs
3. **Verbose toggle** - Include extended details per event

**Configuration Example:**
```json
{
  "enabledCategories": ["AUTHENTICATION", "AUTHORIZATION", "SESSION"],
  "eventFilters": {
    "EID-1001": { "enabled": true, "includeVerbose": false },
    "EID-2001": { "enabled": true, "includeVerbose": true },
    "EID-5001": { "enabled": false }
  }
}
```

### REQ-3: Flexible CSV Column Selection

The system shall support configurable column sets for different use cases:

#### Standard Column Set (Default)
| Column | Required | Description |
|--------|----------|-------------|
| Timestamp | Yes | ISO 8601 UTC format |
| EventID | Yes | EID-XXXX identifier |
| Category | Yes | Event category name |
| Severity | Yes | Critical/Error/Warning/Info/Verbose |
| Outcome | Yes | Success/Failure/Partial |
| Username | No | Authenticated user |
| Action | No | Operation performed |
| Message | Yes | Human-readable description |

#### Extended Column Set (Verbose)
Adds: Domain, SourceIP, ProcessID, ThreadID, SessionID, Resource, Reason, StackTrace

#### Minimal Column Set (Compliance)
Timestamp, EventID, Outcome, Username, Action

### REQ-4: CSV Output Format

**Header Row:**
```csv
Timestamp,EventID,Category,Severity,Outcome,Username,Domain,SourceIP,Action,Resource,Reason,SessionID,ProcessID,Message
```

**Example Rows:**
```csv
2026-03-26T14:23:15.123Z,EID-1001,AUTHENTICATION,INFO,Success,USER1,WORKSTATION1,,CredentialProvider,Logon,,Successful,EID-SESSION-001,1234,User logged in with smart card
2026-03-26T14:25:30.456Z,EID-2001,AUTHORIZATION,ERROR,Failure,USER1,WORKSTATION1,,CredentialProvider,Authenticate,PIN,Incorrect PIN,EID-SESSION-001,1234,PIN verification failed (3 attempts remaining)
2026-03-26T14:30:00.789Z,EID-4001,CERTIFICATE,WARNING,Partial,USER1,WORKSTATION1,,CertificateValidate,Chain,Certificate expiring in 7 days,EID-SESSION-001,1234,Cert expires: 2026-04-02
```

### REQ-5: Configuration Management

**Configuration File Location:**
- Path: `C:\ProgramData\EIDAuthentication\logging.json`
- Fallback: Registry at `HKLM\SOFTWARE\EIDAuthentication\Logging`

**Configuration Schema:**
```json
{
  "version": "1.0",
  "csvFormat": {
    "enabled": true,
    "outputFile": "C:\\Windows\\System32\\LogFiles\\WMI\\EIDCredentialProvider.csv",
    "delimiter": ",",
    "includeHeader": true,
    "columns": ["Timestamp", "EventID", "Category", "Severity", "Outcome", "Username", "Action", "Message"]
  },
  "etwFormat": {
    "enabled": true,
    "outputFile": "C:\\Windows\\System32\\LogFiles\\WMI\\EIDCredentialProvider.etl"
  },
  "filters": {
    "enabledCategories": ["AUTHENTICATION", "AUTHORIZATION", "SESSION", "CERTIFICATE", "AUDIT"],
    "eventFilters": {
      "EID-1001": { "enabled": true, "includeVerbose": false },
      "EID-1002": { "enabled": true, "includeVerbose": true }
    }
  },
  "rotation": {
    "maxSizeMB": 64,
    "maxFiles": 5,
    "compressOldFiles": false
  }
}
```

---

## Event ID Definitions

### AUTHENTICATION (EID-1xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-1001 | Logon Success | Info | User successfully logged in | Card reader, Certificate subject, Logon type |
| EID-1002 | Logon Failure | Error | Logon attempt failed | Failure reason, Attempt count, Reader |
| EID-1003 | Card Detected | Verbose | Smart card detected by reader | Reader name, ATR, Card ID |
| EID-1004 | PIN Verified | Info | PIN entry successful | Attempts remaining |
| EID-1005 | PIN Failed | Warning | PIN entry failed | Attempts remaining, Lockout status |
| EID-1006 | Certificate Validated | Info | Certificate chain validated | Subject, Issuer, Expiry |
| EID-1007 | Certificate Expired | Error | Certificate has expired | Expiry date, Subject |
| EID-1008 | Certificate Not Yet Valid | Warning | Certificate validity in future | Valid from, Subject |

### AUTHORIZATION (EID-2xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-2001 | LSA Secret Read | Info | LSA secret read successfully | Secret name, RID |
| EID-2002 | LSA Secret Write | Info | LSA secret written | Secret name, RID, Data size |
| EID-2003 | LSA Secret Not Found | Warning | LSA secret does not exist | Secret name, RID |
| EID-2004 | Credential Access Denied | Error | Access to credential denied | Requested operation, User |
| EID-2005 | Credential Store Access | Info | Credential provider accessed | Store type, Operation |

### SESSION (EID-3xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-3001 | Session Started | Info | User session initiated | Session ID, Logon type |
| EID-3002 | Session Ended | Info | User session terminated | Session ID, Duration |
| EID-3003 | Session Locked | Info | Workstation locked | Session ID |
| EID-3004 | Session Unlocked | Info | Workstation unlocked | Session ID, Unlock method |
| EID-3005 | Session Renewed | Info | Session refreshed | Session ID, New expiry |

### CERTIFICATE (EID-4xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-4001 | Certificate Validated | Info | Certificate chain validated | Subject, Issuer, Chain length |
| EID-4002 | Certificate Expiring Soon | Warning | Certificate expires soon | Days until expiry, Subject |
| EID-4003 | Certificate Installed | Info | Certificate installed to store | Store name, Subject |
| EID-4004 | Certificate Chain Error | Error | Chain validation failed | Error code, Certificate |

### SMARTCARD (EID-5xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-5001 | Card Inserted | Verbose | Smart card inserted | Reader name, Card ID |
| EID-5002 | Card Removed | Verbose | Smart card removed | Reader name |
| EID-5003 | Reader Connected | Verbose | Card reader attached | Reader name, Device ID |
| EID-5004 | Reader Disconnected | Warning | Card reader detached | Reader name |
| EID-5005 | Card ATR Read | Verbose | Answer To Reset read | ATR value, Reader |
| EID-5006 | Card Communication Failed | Error | Cannot communicate with card | Reader, Error code |

### LSA (EID-6xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-6001 | Secret Created | Info | LSA secret created | Secret name, RID |
| EID-6002 | Secret Deleted | Info | LSA secret deleted | Secret name, RID |
| EID-6003 | Secret Format Error | Error | Invalid secret name format | Invalid name, Expected format |

### CONFIG (EID-7xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-7001 | Logging Started | Info | Logging system initialized | Config file, Format |
| EID-7002 | Logging Stopped | Info | Logging system shutdown | Reason |
| EID-7003 | Configuration Changed | Info | Logging configuration modified | Setting, Old value, New value |
| EID-7004 | Log Rotation | Info | Log file rotated | Old file, New file, Size |

### AUDIT (EID-8xxx)

| Event ID | Name | Severity | Description | Verbose Fields |
|----------|------|----------|-------------|----------------|
| EID-8001 | Credential Export | Warning | Credentials exported | Export format, Destination, Count |
| EID-8002 | Credential Import | Warning | Credentials imported | Source format, Count |
| EID-8003 | Administrative Action | Warning | Admin action performed | Action, Target, Admin user |
| EID-8004 | Security Policy Violation | Error | Security policy violation detected | Policy, Details |

---

## Non-Functional Requirements

### NFR-1: Performance

- Log writes MUST be asynchronous to avoid blocking authentication
- Maximum overhead per log entry: 5ms
- Support at least 1000 log entries per second

### NFR-2: Security

- Sensitive data MUST NOT be logged (passwords, PIN values, private keys)
- Log files MUST have restrictive ACLs (Administrators only, by default)
- Configuration file changes require administrative privileges

### NFR-3: Reliability

- Logging failures MUST NOT cause authentication failures
- On disk full, delete oldest rotated log file
- Corrupted configuration files fall back to safe defaults

### NFR-4: Compatibility

- Existing ETW logging continues to work alongside CSV logging
- Configuration changes via GUI update both registry and JSON config
- Windows 7+ support (maintain current OS compatibility)

---

## Implementation Phases

### Phase 1: Core Infrastructure
1. Create configuration file reader/writer
2. Implement CSV log writer with configurable columns
3. Create event ID enumeration and category system
4. Implement event filtering logic

### Phase 2: Event Instrumentation
1. Add logging calls to authentication paths
2. Add logging calls to authorization paths
3. Add logging calls to certificate operations
4. Add logging calls to smart card operations

### Phase 3: GUI Updates
1. Update EIDLogManager dialog for event selection
2. Add column selection interface
3. Add configuration import/export

### Phase 4: Testing & Documentation
1. Unit tests for log writer
2. Integration tests for filtering
3. User documentation

---

## Open Questions

1. **Configuration Format** - JSON vs INI vs Registry-only?
2. **Default Event Set** - Which events should be enabled by default?
3. **Verbose Performance** - What's the performance impact of verbose logging?
4. **SIEM Integration** - Should we support direct Syslog forwarding?
5. **Log Signing** - Should logs be cryptographically signed for tamper evidence?

---

## References

- [OWASP Logging Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Logging_Cheat_Sheet.html)
- [RFC 5424: The Syslog Protocol](https://www.rfc-editor.org/rfc/rfc5424)
- NIST SP 800-92: Guide to Computer Security Log Management
- PCI DSS Requirement 10: Track and monitor all access to network resources and cardholder data

---

*Document prepared for EID Authentication Project*
