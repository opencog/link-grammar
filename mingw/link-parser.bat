%= A link-parser.exe wrapper for MinGW64 =%
%- This wrapper can be invoked from MINGW64 or from a the Windows console. =%
@echo off
setlocal
if defined ProgramW6432 set ProgramFiles=%ProgramW6432%

REM Prepend the MinGW64 binary path
set "PATH=C:\msys64\mingw64\bin;%PATH%"

REM For USE_WORDGRAPH_DISPLAY
REM Path for "dot.exe"
REM set "PATH=C:\cygwin64\bin;%PATH%"
set "PATH=%ProgramFiles(x86)%\Graphviz2.38\bin;%PATH%"
REM Path for "PhotoViewer.dll"
set "PATH=%ProgramFiles%\Windows Photo Viewer;%PATH%"

%debug_cmd% link-parser.exe %*
