#pragma once

#include <Windows.h>
#include "../EIDCardLibrary/EIDCardLibrary.h"

// CenterWindow and SetIcon are declared in EIDCardLibrary/EIDCardLibrary.h

BOOL IsElevated();
BOOL IsCurrentUserBelongToADomain();
