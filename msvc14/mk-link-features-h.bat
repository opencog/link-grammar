@echo off
setlocal DisableDelayedExpansion
mkdir Temp 2>nul

del ..\link-grammar\link-features.h 2>nul
echo %~f0: Info: Creating ..\link-grammar\link-features.h from ..\link-grammar\link-features.h.in

findstr "^LINK_.*_VERSION=" ..\configure.ac > Temp\version.txt
for /F "tokens=*" %%i in (Temp\version.txt) do set %%i
del Temp\version.txt

%= Process the ".in" file, preserving blank lines and exlamation marks =%
for /f "skip=2 tokens=1* delims=]" %%i in ('find /v /n "" ..\link-grammar\link-features.h.in') do (
	set "line=%%j"
	if "%%j"=="" (echo.) else (
		setlocal EnableDelayedExpansion
		set "line=!line:@LINK_MAJOR_VERSION@=%LINK_MAJOR_VERSION%!"
		set "line=!line:@LINK_MINOR_VERSION@=%LINK_MINOR_VERSION%!"
		set "line=!line:@LINK_MICRO_VERSION@=%LINK_MICRO_VERSION%!"
		echo.!line!
		endlocal
	)
)>>..\link-grammar\link-features.h
