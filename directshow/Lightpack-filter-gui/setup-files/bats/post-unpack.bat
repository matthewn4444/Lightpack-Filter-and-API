@echo off
rem Not used for anything else but to copy files to original excutable folder
rem usage: post-unpack.bat <seconds to wait> <temp folder> <execution folder>
if [%1]==[] goto done
if [%2]==[] goto done
if [%3]==[] goto done

rem Windows 7
@timeout /t %1 /nobreak >nul 2>&1

rem Windows XP
if errorlevel 1 ping 192.0.2.2 -n 1 -w %1000 > nul

rem Copy new files
xcopy /s /y %2 %3

del %3\post-unpack.bat

rem Delete the zip folder
del %2\..\lightpack-filter.zip

rem Delete this folder
@RD /S /Q %2

:done
