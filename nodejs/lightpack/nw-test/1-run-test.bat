@echo off
set TOOLS=..\..\..\directshow\Lightpack-filter-gui\setup-files\tools

rem Compiles and places the nw file into the release and debug folders
del nw-test.nw
copy ..\..\..\Release\lightpack.node lightpack.node
%TOOLS%\zip nw-test.nw index.html package.json *.js lightpack.node

start ../../nw-binaries/nw.exe nw-test.nw
