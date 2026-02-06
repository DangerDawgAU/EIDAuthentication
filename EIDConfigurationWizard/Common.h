#pragma once

#include <windows.h>

// Utility functions for window management
VOID CenterWindow(HWND hWnd);
VOID SetIcon(HWND hWnd);
BOOL IsElevated();
BOOL IsCurrentUserBelongToADomain();
