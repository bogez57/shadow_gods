@echo off

if not defined DevEnvDir ( call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" )

w:

REM %~dp0 represents the full directory path to your batch file 
set cwd=%~dp0\

IF NOT EXIST build mkdir build
pushd build

cl -EHsc -I %cwd%tests -I %cwd% -I %cwd%"third_party/boagz/include" ../tests/test_main.cpp ../source/memory_handling.cpp -MP -nologo /link -out:tests.exe

REM Run tests
tests.exe

popd