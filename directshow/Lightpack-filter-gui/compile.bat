@echo off
rem File paths
set SETUP=setup-files
set NW_BIN=..\..\nodejs\nw-binaries
set RELEASE=..\..\Release
set DEBUG=..\..\Debug

rem Compiles and places the app files into the release and debug folders
del %RELEASE%\setup.exe
del %RELEASE%\setup.iss
@RD /S /Q "%RELEASE%\app"
@RD /S /Q "%DEBUG%\app"
copy %RELEASE%\lightpack.node app\node_modules\lightpack\lightpack.node
copy %RELEASE%\app-mutex.node app\node_modules\lightpack\app-mutex.node

rem Copy the nw.exe, nw.pak, app folder and icudt.dll file to those folders
copy package.json %RELEASE%\package.json
copy package.json %DEBUG%\package.json
copy %NW_BIN%\nw.exe %RELEASE%\nw.exe
copy %NW_BIN%\nw.exe %DEBUG%\nw.exe
copy %NW_BIN%\nw.pak %RELEASE%\nw.pak
copy %NW_BIN%\nw.pak %DEBUG%\nw.pak
copy %NW_BIN%\icudt.dll %RELEASE%\icudt.dll
copy %NW_BIN%\icudt.dll %DEBUG%\icudt.dll
xcopy /s /y %SETUP%\files %RELEASE%\
xcopy /s /y app %RELEASE%\app\
xcopy /s /y app %DEBUG%\app\
copy ..\..\LICENSE.txt %RELEASE%\LICENSE.txt

rem Move the bat files (for registering/unregistering the filter)
copy %SETUP%\bats\installFilter.bat %RELEASE%\installFilter.bat
copy %SETUP%\bats\uninstallFilter.bat %RELEASE%\uninstallFilter.bat
