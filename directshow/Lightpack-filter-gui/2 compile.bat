rem File paths
set SETUP=setup-files
set RELEASE=..\..\Release
set DEBUG=..\..\Debug

rem Compiles and places the nw file into the release and debug folders
del %RELEASE%\nw.exe
del %RELEASE%\setup.iss
copy %RELEASE%\lightpack.node node_modules\lightpack\lightpack.node
zip -r app.nw package.json node_modules\* src\*

rem Create the single executable and put it in release & debug
copy /b %SETUP%\nw\nw.exe+app.nw %RELEASE%\nw.exe
copy "%RELEASE%\nw.exe" "%DEBUG%\nw.exe"

rem Copy the nw.pak and icudt.dll file to those folders
copy src\images\icon.ico %RELEASE%\icon.ico
copy %SETUP%\nw\nw.pak %RELEASE%\nw.pak
copy %SETUP%\nw\nw.pak %DEBUG%\nw.pak
copy %SETUP%\nw\icudt.dll %RELEASE%\icudt.dll
copy %SETUP%\nw\icudt.dll %DEBUG%\icudt.dll
copy %SETUP%\setup.iss %RELEASE%\setup.iss

rem Move the bat files (for registering/unregistering the filter)
copy %SETUP%\bats\installFilter.bat %RELEASE%\installFilter.bat
copy %SETUP%\bats\uninstallFilter.bat %RELEASE%\uninstallFilter.bat

rem Clean the app file
del app.nw
