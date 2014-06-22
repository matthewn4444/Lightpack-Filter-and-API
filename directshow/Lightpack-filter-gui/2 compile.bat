rem Compiles and places the nw file into the release and debug folders
del ..\..\Release\nw.exe
copy ..\..\Release\lightpack.node node_modules\lightpack\lightpack.node
zip -r app.nw package.json node_modules\* src\*

rem Create the single executable and put it in release & debug
copy /b nw-binaries\nw.exe+app.nw ..\..\Release\nw.exe
copy "..\..\Release\nw.exe" "..\..\Debug\nw.exe"

rem Copy the nw.pak and icudt.dll file to those folders
copy /b nw-binaries\nw.pak ..\..\Release\nw.pak
copy /b nw-binaries\nw.pak ..\..\Debug\nw.pak
copy /b nw-binaries\icudt.dll ..\..\Release\icudt.dll
copy /b nw-binaries\icudt.dll ..\..\Debug\icudt.dll

rem Clean the app file
del app.nw
