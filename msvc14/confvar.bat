@echo off
%= Filter - replace the *VERSION atoconf variables according to the ones   =%
%= set in configure.ac.                                                    =%

setlocal DisableDelayedExpansion
mkdir Temp 2>nul

set conf=..\configure.ac
set tmp=Temp\version.txt

echo 1>&2 %~f0: Info: Replacing configuration variables

findstr "^LINK_.*_VERSION=" %conf% >  %tmp%
findstr "^VERSION="         %conf% >> %tmp%
for /F "tokens=*" %%i in (%tmp%) do set %%i
del %tmp%

%= Process the ".in" file, preserving blank lines and exlamation marks =%
for /f "tokens=1* delims=]" %%i in ('find /v /n ""') do (
	set "line=%%j"
	if "%%j"=="" (echo.) else (
		setlocal EnableDelayedExpansion
		set "line=!line:@LINK_MAJOR_VERSION@=%LINK_MAJOR_VERSION%!"
		set "line=!line:@LINK_MINOR_VERSION@=%LINK_MINOR_VERSION%!"
		set "line=!line:@LINK_MICRO_VERSION@=%LINK_MICRO_VERSION%!"
		set "line=!line:@VERSION@=%VERSION%!"

		set "line=!line:$LINK_MAJOR_VERSION=%LINK_MAJOR_VERSION%!"
		set "line=!line:$LINK_MINOR_VERSION=%LINK_MINOR_VERSION%!"
		set "line=!line:$LINK_MICRO_VERSION=%LINK_MICRO_VERSION%!"

		echo(!line!
		endlocal
	)
)
