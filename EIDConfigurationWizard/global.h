
extern HINSTANCE g_hinst;  // NOSONAR - RUNTIME-01: HINSTANCE set by Windows

extern BOOL fShowNewCertificatePanel;  // NOSONAR - RUNTIME-01: UI state flag
extern BOOL fGotoNewScreen;  // NOSONAR - RUNTIME-01: UI navigation flag

extern WCHAR szReader[];  // NOSONAR - RUNTIME-01: Buffer filled at runtime
extern const DWORD dwReaderSize;
extern WCHAR szCard[];  // NOSONAR - RUNTIME-01: Buffer filled at runtime
extern const DWORD dwCardSize;
extern WCHAR szUserName[];  // NOSONAR - RUNTIME-01: Buffer filled at runtime
extern const DWORD dwUserNameSize;
extern WCHAR szPassword[];  // NOSONAR - RUNTIME-01: Buffer filled at runtime
extern const DWORD dwPasswordSize;
VOID SecurelyClearPassword();
extern WCHAR szCAName[];  // NOSONAR - RUNTIME-01: Buffer for custom CA name
extern const DWORD dwCANameSize;


VOID CenterWindow(HWND hWnd);
BOOL IsElevated();
BOOL IsCurrentUserBelongToADomain();
BOOL DialogForceSmartCardLogonPolicy();
BOOL DialogRemovePolicy();
BOOL CreateDebugReport(PTSTR szLogFile);
VOID CreateReport(PTSTR szNamedPipeName);
// SendReport removed - internet reporting functionality disabled
VOID SetIcon(HWND hWnd);
