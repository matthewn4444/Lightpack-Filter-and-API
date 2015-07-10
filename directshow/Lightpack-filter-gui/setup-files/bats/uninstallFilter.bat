rem Unregisters the Lightpack Filter
cd /d "%~dp0"
regsvr32 /u /s lightpack.ax
regsvr32 /u /s lightpack64.ax
