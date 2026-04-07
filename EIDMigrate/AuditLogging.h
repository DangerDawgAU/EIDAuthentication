#pragma once

// File: EIDMigrate/AuditLogging.h
// Audit logging to Windows Event Log and file

#include "EIDMigrate.h"
#include <string>

// Audit event types
enum class EID_AUDIT_EVENT_TYPE : DWORD
{
    EXPORT_SUCCESS = 1,
    EXPORT_FAILURE = 2,
    IMPORT_SUCCESS = 3,
    IMPORT_FAILURE = 4,
    ACCESS_DENIED = 5,
    AUTH_FAILURE = 6,
    VALIDATION_SUCCESS = 7,
    VALIDATION_FAILURE = 8,
    WARNING = 9,
    // BUG FIX #21: Additional audit event types for security monitoring
    USER_CREATED = 10,
    GROUP_CREATED = 11,
    CERTIFICATE_INSTALLED = 12,
    CERTIFICATE_VALIDATION_FAILED = 13,
    RATE_LIMIT_EXCEEDED = 14,
    CREDENTIAL_EXPORT_START = 15,
    CREDENTIAL_IMPORT_START = 16,
    LSA_ACCESS = 17,
};

// Initialize audit logging
HRESULT InitializeAuditLogging();

// Log audit event to Windows Event Log
void LogAuditEvent(
    _In_ EID_AUDIT_EVENT_TYPE eventType,
    _In_opt_ PCWSTR pwszUsername,
    _In_opt_ PCWSTR pwszDetails);

// Log audit event to file
void LogAuditEventToFile(
    _In_ EID_AUDIT_EVENT_TYPE eventType,
    _In_opt_ PCWSTR pwszUsername,
    _In_opt_ PCWSTR pwszDetails);

// Log both to Event Log and file
void LogAuditEventBoth(
    _In_ EID_AUDIT_EVENT_TYPE eventType,
    _In_opt_ PCWSTR pwszUsername,
    _In_opt_ PCWSTR pwszDetails);

// Set log file path
HRESULT SetLogFile(_In_ const std::wstring& wsFilePath);

// Close log file
void CloseLogFile();

// Flush log file
void FlushLogFile();

// Get event type as string
PCWSTR GetEventTypeString(_In_ EID_AUDIT_EVENT_TYPE eventType);

// Get event type as log level
WORD GetEventLogLevel(_In_ EID_AUDIT_EVENT_TYPE eventType);

// Convenience macros
#define LOG_SUCCESS_EXPORT(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::EXPORT_SUCCESS, user, details)
#define LOG_FAILURE_EXPORT(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::EXPORT_FAILURE, user, details)
#define LOG_SUCCESS_IMPORT(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::IMPORT_SUCCESS, user, details)
#define LOG_FAILURE_IMPORT(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::IMPORT_FAILURE, user, details)
#define LOG_ACCESS_DENIED(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::ACCESS_DENIED, user, details)
#define LOG_WARNING(details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::WARNING, nullptr, details)
// BUG FIX #21: Additional audit macros for security monitoring
#define LOG_USER_CREATED(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::USER_CREATED, user, details)
#define LOG_GROUP_CREATED(group, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::GROUP_CREATED, group, details)
#define LOG_CERTIFICATE_INSTALLED(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::CERTIFICATE_INSTALLED, user, details)
#define LOG_CERTIFICATE_VALIDATION_FAILED(user, details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::CERTIFICATE_VALIDATION_FAILED, user, details)
#define LOG_RATE_LIMIT_EXCEEDED(details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::RATE_LIMIT_EXCEEDED, nullptr, details)
#define LOG_EXPORT_START(details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::CREDENTIAL_EXPORT_START, nullptr, details)
#define LOG_IMPORT_START(details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::CREDENTIAL_IMPORT_START, nullptr, details)
#define LOG_LSA_ACCESS(details) \
    LogAuditEventBoth(EID_AUDIT_EVENT_TYPE::LSA_ACCESS, nullptr, details)
