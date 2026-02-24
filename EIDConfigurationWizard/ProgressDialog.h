#pragma once
#include <Windows.h>

// Shows the progress dialog modally on the parent window
// Returns: Handle to the progress dialog window, or NULL on failure
HWND ShowProgressDialog(HWND hParentWnd);

// Closes and destroys the progress dialog
void CloseProgressDialog(HWND hProgressDialog);
