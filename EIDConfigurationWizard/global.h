
extern HINSTANCE g_hinst;

extern BOOL fShowNewCertificatePanel;
extern BOOL fGotoNewScreen;

extern WCHAR szReader[];
extern DWORD dwReaderSize;
extern WCHAR szCard[];
extern DWORD dwCardSize;
extern WCHAR szUserName[];
extern DWORD dwUserNameSize;
extern WCHAR szPassword[];
extern DWORD dwPasswordSize;


VOID CenterWindow(HWND hWnd);
BOOL IsElevated();
BOOL IsCurrentUserBelongToADomain();
BOOL DialogForceSmartCardLogonPolicy();
BOOL DialogRemovePolicy();
BOOL CreateDebugReport(PTSTR szLogFile);
VOID CreateReport(PTSTR szNamedPipeName);
// SendReport removed - internet reporting functionality disabled
VOID SetIcon(HWND hWnd);
