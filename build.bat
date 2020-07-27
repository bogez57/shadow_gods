@echo off

misc\ctime -begin timings_file_for_this_build.ctm

if not defined DevEnvDir ( call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" )

w: REM since above call to vcvars seems to change directories we need to chang back to this project dir

REM %~dp0 represents the full directory path to your batch file
set cwd=%~dp0\

set PreProcessorSwitches=-DDEVELOPMENT_BUILD=1 -DGLEW_STATIC=1

REM Debug/Development build
set CommonCompilerFlags=-Gm- -MP -Z7 -nologo -Oi -Od -Ob1 -WX -W3 -GR -EHa- -arch:AVX2 -std:c++17 -wd4505 -wd4101 -wd4530 -w14700
set CommonLinkerFlags=-subsystem:windows -machine:x64 -incremental:no -nologo -opt:ref -debug -ignore:4099 -STACK:2000000

set GameIncludePaths=-I %cwd%"third_party/boagz/include" -I %cwd%"third_party/boagz/src" -I %cwd%"third_party/stb/include" -I %cwd%"third_party/micro_parser_combinators"
set GameLibraryPaths=

set PlatformIncludePaths=-I %cwd%"third_party/boagz/include" -I %cwd%"third_party/boagz/src" -I %cwd%"third_party/glew-2.1.0/include" -I %cwd%"third_party/stb/include"
set PlatformLibraryPaths=-LIBPATH:%cwd%"third_party/glew-2.1.0/lib/win64-release"
set PlatformImportLibraries="user32.lib" "OpenGL32.lib" "gdi32.lib" "xinput.lib" "Winmm.lib"
set PlatformStaticLibraries="glew32s.lib"

IF NOT EXIST build mkdir build
pushd build

REM build debug lib
cl /c ..\source\debug\debug.cpp %CommonCompilerFlags%
link debug.obj -dll -OUT:debugdll.dll -debug -machine:x64 -incremental:no -nologo -opt:ref

REM Clear out pdb files so build directory doesn't get too huge and build app DLL
REM del *.pdb > NUL 2> NUL
cl /c ..\source\gamecode.cpp %cwd%third_party/micro_parser_combinators/mpc/mpc.c  %GameIncludePaths% %CommonCompilerFlags% %PreProcessorSwitches%
link gamecode.obj mpc.obj debugdll.lib -dll -PDB:gamecode_%random%.pdb -export:GameUpdate -export:DebugFrameEnd %GameLibraryPaths% %CommonLinkerFlags%

REM Build exe
cl /c ..\source\win64_shadowgods.cpp %CommonCompilerFlags% %PlatformIncludePaths% %PreProcessorSwitches%
link win64_shadowgods.obj debugdll.lib -OUT:win64_shadowgods.exe %CommonLinkerFlags% %PlatformLibraryPaths% %PlatformImportLibraries% %PlatformStaticLibraries%

popd

misc\ctime -end timings_file_for_this_build.ctm