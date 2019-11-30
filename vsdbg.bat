@echo off

tasklist /FI "IMAGENAME eq devenv.exe" 2>NUL | find /I /N "devenv.exe">NUL

if "%ERRORLEVEL%"=="0" (
    REM Program is already running
) else (
    devenv build\win64_shadowgods.exe source\gamecode.cpp
)
