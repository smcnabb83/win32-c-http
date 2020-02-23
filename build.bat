@echo off

IF "%VSCMD_VER%" == "" (call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat")

rem move the below flags to CommonCompilerFlags for debug mode.
rem -D DEBUG -Zi

set CommonCompilerFlags=-MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4996 -FS
set CommonLinkerFlags= -incremental:no -opt:ref -PDB:server.pdb user32.lib gdi32.lib winmm.lib ws2_32.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\src\server.c ..\src\request.c ..\src\response.c ..\src\header.c ..\src\error.c ..\src\defaults.c ..\src\ringbuf.c /link %CommonLinkerFlags%
popd

xcopy *.html .\build\. /Y
xcopy *.css .\build\. /Y
xcopy *.png .\build\. /Y