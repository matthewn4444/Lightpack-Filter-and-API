@echo off

rem Compiles and places the nw file into the release and debug folders
del nw-test.nw
copy ..\..\..\Release\app-mutex.node app-mutex.node
zip nw-test.nw index.html package.json *.js app-mutex.node

start ../../nw-binaries/nw.exe nw-test.nw
