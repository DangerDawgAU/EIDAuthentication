//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by EIDMigrateUI.rc
// NOSONAR - Macros are required by Windows Resource Compiler; cannot use constexpr/enum for resource IDs
//
#define IDI_APP_ICON                    200 // NOSONAR - RESOURCE-01: RC.exe requires #define macros (use 200 to avoid conflicts)

// Dialog IDs
#define IDD_01_WELCOME                  100
#define IDD_02_EXPORT_SELECT            101
#define IDD_03_EXPORT_CONFIRM           102
#define IDD_04_EXPORT_PROGRESS          103
#define IDD_05_EXPORT_COMPLETE          104
#define IDD_06_IMPORT_SELECT            105
#define IDD_07_IMPORT_OPTIONS           106
#define IDD_08_IMPORT_PREVIEW            107
#define IDD_09_IMPORT_PROGRESS          108
#define IDD_10_IMPORT_COMPLETE          109
#define IDD_11_LIST_CREDENTIALS         110
#define IDD_12_VALIDATE_FILE            111
#define IDD_PROGRESS                    112
#define IDD_13_PASSWORD_PROMPT          113

// Control IDs - Welcome Page
#define IDC_01_EXPORT                   1001
#define IDC_01_IMPORT                   1002
#define IDC_01_LIST                     1003
#define IDC_01_VALIDATE                 1004
#define IDC_STC_WELCOME                 1005

// Control IDs - Export Select Page
#define IDC_02_OUTPUT_FILE              2001
#define IDC_02_BROWSE_OUTPUT            2002
#define IDC_02_PASSWORD                 2003
#define IDC_02_CONFIRM_PASSWORD         2004
#define IDC_02_SHOW_PASSWORD            2005
#define IDC_02_VALIDATE_CERTS           2006
#define IDC_02_INCLUDE_GROUPS           2007
#define IDC_02_OUTPUT_LABEL             2008
#define IDC_02_PASSWORD_LABEL           2009
#define IDC_02_CONFIRM_LABEL            2010

// Control IDs - Export Confirm Page
#define IDC_03_SUMMARY                  3001
#define IDC_03_CREDENTIAL_COUNT         3002
#define IDC_03_OUTPUT_PATH              3003
#define IDC_03_ENCRYPTION_INFO          3004
#define IDC_03_SHIELD                   3005

// Control IDs - Export Progress Page
#define IDC_04_PROGRESSTEXT             4001
#define IDC_04_PROGRESSBAR              4002

// Control IDs - Export Complete Page
#define IDC_05_SUCCESS_TEXT             5001
#define IDC_05_TOTAL_CREDENTIALS        5002
#define IDC_05_CERT_ENCRYPTED           5003
#define IDC_05_DPAPI_ENCRYPTED          5004
#define IDC_05_GROUPS_EXPORTED          5005
#define IDC_05_OUTPUT_PATH              5006
#define IDC_05_OPEN_FOLDER              5007
#define IDC_05_SHIELD                   5008

// Control IDs - Import Select Page
#define IDC_06_INPUT_FILE               6001
#define IDC_06_BROWSE_INPUT             6002
#define IDC_06_PASSWORD                 6003
#define IDC_06_SHOW_PASSWORD            6004
#define IDC_06_DECRYPT_BUTTON           6005
#define IDC_06_FILE_INFO                6006
#define IDC_06_SOURCE_MACHINE           6007
#define IDC_06_EXPORT_DATE              6008
#define IDC_06_CREDENTIAL_COUNT         6009
#define IDC_06_VERSION                  6010
#define IDC_06_INPUT_LABEL              6011
#define IDC_06_PASSWORD_LABEL           6012

// Control IDs - Import Options Page
#define IDC_07_DRY_RUN                  7001
#define IDC_07_FORCE_IMPORT             7002
#define IDC_07_CREATE_USERS             7003
#define IDC_07_CONTINUE_ON_ERROR        7004
#define IDC_07_OVERWRITE                7005
#define IDC_07_WARNING_TEXT             7006
#define IDC_07_OPTIONS_GROUP            7007

// Control IDs - Import Preview Page
#define IDC_08_LIST                     8001
#define IDC_08_WARNINGS                 8002
#define IDC_08_CONTINUE                 8003
#define IDC_08_SHIELD                   8004
#define IDC_08_PREVIEW_TEXT             8005

// Control IDs - Import Progress Page
#define IDC_09_PROGRESSTEXT             9001
#define IDC_09_PROGRESSBAR              9002
#define IDC_09_CURRENT_USER             9003

// Control IDs - Import Complete Page
#define IDC_10_SUMMARY_TEXT             10001
#define IDC_10_TOTAL                    10002
#define IDC_10_IMPORTED                 10003
#define IDC_10_FAILED                   10004
#define IDC_10_USERS_CREATED            10005
#define IDC_10_GROUPS_CREATED           10006
#define IDC_10_WARNINGS                 10007
#define IDC_10_SHIELD                   10008
#define IDC_10_VIEW_LOG                 10009

// Control IDs - List Credentials Page
#define IDC_11_LOCAL                    11001
#define IDC_11_FROM_FILE                11002
#define IDC_11_FILE_BROWSE              11003
#define IDC_11_LIST                     11004
#define IDC_11_REFRESH                  11005
#define IDC_11_EXPORT_SELECTED          11006
#define IDC_11_DETAILS                  11007

// Control IDs - Validate File Page
#define IDC_12_FILE_PATH                12001
#define IDC_12_BROWSE                   12002
#define IDC_12_VALIDATE                 12003
#define IDC_12_PASSWORD                 12013
#define IDC_12_SHOW_PASSWORD            12014
#define IDC_12_RESULTS                  12004
#define IDC_12_FORMAT_STATUS            12005
#define IDC_12_HEADER_STATUS            12006
#define IDC_12_HMAC_STATUS              12007
#define IDC_12_ENCRYPTION_STATUS        12008
#define IDC_12_JSON_STATUS              12009
#define IDC_12_CREDENTIAL_COUNT         12010
#define IDC_12_SOURCE_MACHINE           12011
#define IDC_12_EXPORT_DATE              12012

// Control IDs - Password Prompt Dialog
#define IDC_13_USERNAME                 13001
#define IDC_13_PASSWORD                 13002
#define IDC_13_CONFIRM_PASSWORD         13003
#define IDC_13_SHOW_PASSWORD            13004
#define IDC_13_SKIP                     13005
#define IDC_13_INFO_TEXT                13006

// Common controls
#define IDC_STATIC                      -1

// String IDs
#define IDS_CAPTION                     1
#define IDS_TITLE_WELCOME               2
#define IDS_TITLE_EXPORT_SELECT         3
#define IDS_TITLE_EXPORT_CONFIRM        4
#define IDS_TITLE_EXPORT_COMPLETE       5
#define IDS_TITLE_IMPORT_SELECT         6
#define IDS_TITLE_IMPORT_OPTIONS        7
#define IDS_TITLE_IMPORT_PREVIEW        8
#define IDS_TITLE_IMPORT_COMPLETE       9
#define IDS_TITLE_LIST                  10
#define IDS_TITLE_VALIDATE              11

#define IDS_EID_NOT_AVAILABLE           50
#define IDS_REQUIRE_ADMIN               51

#define IDS_ERROR_INVALID_PASSWORD      100
#define IDS_ERROR_PASSWORD_MISMATCH     101
#define IDS_ERROR_NO_CREDENTIALS        102
#define IDS_ERROR_FILE_WRITE            103
#define IDS_ERROR_FILE_READ             104
#define IDS_ERROR_DECRYPT_FAILED        105
#define IDS_ERROR_INVALID_FILE          106

#define IDS_WARNING_SHORT_PASSWORD      200
#define IDS_WARNING_OVERWRITE           201
#define IDS_WARNING_MISSING_USERS       202

#define IDS_INFO_EXPORT_COMPLETE        300
#define IDS_INFO_IMPORT_COMPLETE        301
#define IDS_INFO_CREATING_USERS         302

// Next default values for new objects
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        113
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         12013
#define _APS_NEXT_SYMED_VALUE           241
#endif
#endif
