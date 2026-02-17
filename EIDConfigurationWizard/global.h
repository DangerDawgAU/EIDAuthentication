
extern HINSTANCE g_hinst;

extern BOOL fShowNewCertificatePanel;
extern BOOL fGotoNewScreen;

extern WCHAR szReader[];
extern const DWORD dwReaderSize;
extern WCHAR szCard[];
extern const DWORD dwCardSize;
extern WCHAR szUserName[];
extern const DWORD dwUserNameSize;
extern WCHAR szPassword[];
extern const DWORD dwPasswordSize;


VOID CenterWindow(HWND hWnd);
BOOL IsElevated();
BOOL IsCurrentUserBelongToADomain();
BOOL DialogForceSmartCardLogonPolicy();
BOOL DialogRemovePolicy();
BOOL CreateDebugReport(PTSTR szLogFile);
VOID CreateReport(PTSTR szNamedPipeName);
// SendReport removed - internet reporting functionality disabled
VOID SetIcon(HWND hWnd);
