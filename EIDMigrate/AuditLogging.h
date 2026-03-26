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
