%= A link-parser.exe wrapper for Cygwin =%
%= This wrapper can be invoked from Cygwin or from a the Windows console. =%

@echo off

if x%1==x-b (
	if x%2==x (
		echo Usage: %0 -b BUILD_DIR [link-parser arguments]
		exit /B 2
	)
	if not exist %2\config.log (
		echo "Usage: %0 -b BUILD_DIR [link-parser arguments]"
		exit /B 2
	)
	set "PATH=%2\link-parser;%PATH%"
	shift
	shift
)

setlocal
if defined ProgramW6432 set ProgramFiles=%ProgramW6432%

REM Add the Cygwin binary path
set "PATH=C:\cygwin64\bin;\cygwin64\usr\local\bin;%PATH%"

REM For USE_WORDGRAPH_DISPLAY
if not exist C:\cygwin64\bin\dot.exe (
	set "PATH=%ProgramFiles(x86)%\Graphviz2.38\bin;%PATH%"
)
REM Path for "PhotoViewer.dll"
set "PATH=%ProgramFiles%\Windows Photo Viewer;%PATH%"

path
set /p=Starting <nul
which link-parser.exe

%debug_cmd% link-parser.exe %1 %2 %3 %4 %5 %6 %7 %8 %9
