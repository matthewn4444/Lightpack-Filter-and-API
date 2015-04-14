@echo off
rem File paths
set SETUP=setup-files
set NW_BIN=..\..\nodejs\nw-binaries
set RELEASE=..\..\Release
set DEBUG=..\..\Debug

rem Compiles and places the nw file into the release and debug folders
del %RELEASE%\nw.exe
del %RELEASE%\setup.exe
del %RELEASE%\setup.iss
copy %RELEASE%\lightpack.node node_modules\lightpack\lightpack.node
copy %RELEASE%\app-mutex.node node_modules\lightpack\app-mutex.node
%SETUP%\tools\zip -r app.nw package.json node_modules\* src\*

rem Create the single executable and put it in release & debug
copy /b %NW_BIN%\nw.exe+app.nw %RELEASE%\nw.exe
copy "%RELEASE%\nw.exe" "%DEBUG%\nw.exe"

rem Copy the nw.pak and icudt.dll file to those folders
copy src\images\icon.ico %RELEASE%\icon.ico
copy %NW_BIN%\nw.pak %RELEASE%\nw.pak
copy %NW_BIN%\nw.pak %DEBUG%\nw.pak
copy %NW_BIN%\icudt.dll %RELEASE%\icudt.dll
copy %NW_BIN%\icudt.dll %DEBUG%\icudt.dll
xcopy /s /y %SETUP%\files %RELEASE%\
copy ..\..\LICENSE.txt %RELEASE%\LICENSE.txt

rem Move the bat files (for registering/unregistering the filter)
copy %SETUP%\bats\installFilter.bat %RELEASE%\installFilter.bat
copy %SETUP%\bats\uninstallFilter.bat %RELEASE%\uninstallFilter.bat

rem Clean the app file
del app.nw
