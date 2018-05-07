@echo off

if not defined DevEnvDir ( call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat" )

REM %~dp0 represents the full directory path to your batch file 
chdir /D %~dp0

set cwd=%~dp0\

set PreProcessorSwitches=-DDEVELOPMENT_BUILD=1 

REM If you need a console window: -subsytem:console ::: otherwise: -subsystem:windows plus -entry:mainCRTStartup
REM Debug/Development build 
set CommonCompilerFlags=-Gm- -MP -Z7 -nologo -Oi -Od -WX -W3 -GR -EHa- -std:c++17 -wd4505
set CommonLinkerFlags=-subsystem:windows -machine:x64 -incremental:no -nologo -opt:ref -debug:fastlink 

set IncludePaths=-I %cwd%"third_party/boagz/include" -I %cwd%"third_party/boagz/src" 
set LibraryPaths=

set ImportLibraries="user32.lib" "OpenGL32.lib" "gdi32.lib" "xinput.lib" 
set StaticLibraries=

IF NOT EXIST build mkdir build
pushd build

REM Clear out pdb files so build directory doesn't get too huge and build app DLL
del *.pdb > NUL 2> NUL
cl /c ..\source\gamecode.cpp %CommonCompilerFlags% %IncludePaths% %PreProcessorSwitches%
link gamecode.obj -dll -PDB:gamecode_%random%.pdb -export:GameUpdate %CommonLinkerFlags% %LibraryPaths%

REM Build exe
cl /c ..\source\win64_shadowgods.cpp %CommonCompilerFlags% %IncludePaths% %PreProcessorSwitches% 
link win64_shadowgods.obj -OUT:win64_shadowgods.exe %CommonLinkerFlags% %LibraryPaths% %ImportLibraries% %StaticLibraries%

popd