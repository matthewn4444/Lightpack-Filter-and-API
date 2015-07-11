@echo off

rem Install Node dependencies
copy package.json app\package.json
call npm install --prefix app
del app\package.json
