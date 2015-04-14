@echo off

rem File paths
set SETUP=setup-files
set NW_BIN=..\..\nodejs\nw-binaries
set RELEASE=..\..\Release
set DEBUG=..\..\Debug
set OLD_DIR=%CD%

call "compile.bat"

rem Create the zip file
copy setup-files\bats\post-unpack.bat %RELEASE%\post-unpack.bat
cd %RELEASE% && %SETUP%\tools\zip -r lightpack-filter icon.ico icudt.dll lightpack.ax nw.exe nw.pak installFilter.bat uninstallFilter.bat post-unpack.bat LICENSE.txt
del post-unpack.bat

cd %OLD_DIR%


pause
