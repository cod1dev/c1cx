@echo off
set "COD_PATH=C:\Call of Duty %PATCH%"
set "BIN_PATH=bin\Debug %PATCH%"
cd "%BIN_PATH%"
copy mss32.dll "%COD_PATH%"