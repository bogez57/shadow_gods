@echo off

if not defined DevEnvDir ( call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" )

w:

REM %~dp0 represents the full directory path to your batch file 
set cwd=%~dp0\

IF NOT EXIST build mkdir build
pushd build

cl /c %cwd%tests/*.cpp -EHsc -I %cwd%"source" -I %cwd%"tests" -I %cwd%"third_party/boagz/include" -MP -nologo -std:c++17 
link *.obj -out:tests.exe

REM Run tests
tests.exe

popd