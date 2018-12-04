@echo off

if not defined DevEnvDir ( call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" )

REM %~dp0 represents the full directory path to your batch file 
chdir /D %~dp0
set cwd=%~dp0\

IF NOT EXIST build mkdir build
pushd build

cl -EHsc -I %cwd%tests ../tests/*.cpp -MP -nologo /link -out:tests.exe

popd