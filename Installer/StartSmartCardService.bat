@echo off
echo Starting Smart Card services...
net start SCardSvr
net start ScDeviceEnum
echo.
echo Checking service status:
sc query SCardSvr
echo.
sc query ScDeviceEnum
pause
