@echo off

REM *** TO USE THIS BUILD.BAT: ***
REM 1: Put src files in src/
REM 2: make build/ directory next to src/
REM 3: put this build.bat next to src/ and build/
REM 4: set ProjectDir and VCVarsLocation and run.

set ProjectDir=F:/CODING/phragdat
set VCVarsLocation="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"

if exist build\phragdat.exe del build\phragdat.exe
call %VCVarsLocation% x64
pushd build
cl -nologo -MT -Gm- -GR- -EHa- -Oi -W4 -FC -std:c++17 -EHsc %ProjectDir%/src/phragdat.cpp /link -subsystem:console -opt:ref Shlwapi.lib Kernel32.lib
popd
exit