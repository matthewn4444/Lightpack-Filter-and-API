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
cd %RELEASE% && "%CD%\%SETUP%\tools\zip" -r lightpack-filter icudt.dll app locales package.json lightpack.ax^
                                             lightpack64.ax nw.exe installFilter.bat uninstallFilter.bat post-unpack.bat LICENSE.txt^
                                             ffmpeg.dll icudtl.dat natives_blob.bin node.dll nw.dll nw_100_percent.pak nw_200_percent.pak^
                                             nw_elf.dll resources.pak

del post-unpack.bat

rem Create setup file
set "LOCATION_X64=C:\Program Files\Inno Setup 6\iscc.exe"
set "LOCATION_X86=C:\Program Files (x86)\Inno Setup 6\iscc.exe"

if exist "%LOCATION_X64%" (
    set "COMPILER=%LOCATION_X64%"
) else if exist "%LOCATION_X86%" (
    set "COMPILER=%LOCATION_X86%"
) else (
    echo Cannot find Inno Setup compiler, did you install it?
    goto done
)

echo Using inno setup compiler from %COMPILER%
"%COMPILER%" "setup.iss"
:done

cd %OLD_DIR%
