@echo off

rem Compiles and places the nw file into the release and debug folders
del ..\..\Release\nw-test.nw
copy ..\..\Release\lightpack.node lightpack.node
zip ..\..\Release\nw-test.nw index.html package.json *.js lightpack.node

start ../../Release/nw.exe ../../Release/nw-test.nw
