%= A link-parser.exe wrapper for Cygwin =%
%= This wrapper can be invoked from Cygwin or from a the Windows console. =%
@echo off
setlocal
if defined ProgramW6432 set ProgramFiles=%ProgramW6432%


REM Add the Cygwin binary path
set "PATH=%PATH%;C:\cygwin64\bin;\cygwin64\usr\local\bin"

REM For USE_WORDGRAPH_DISPLAY
REM "dot.exe, if exists, is in C:\cygwin64\bin
REM set "PATH=%ProgramFiles(x86)%\Graphviz2.38\bin;%PATH%"
REM Path for "PhotoViewer.dll"
set "PATH=%ProgramFiles%\Windows Photo Viewer;%PATH%"
path
which link-parser.exe

%debug_cmd% link-parser.exe %*
