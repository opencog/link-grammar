@echo off
%= Filter - replace the *VERSION atoconf variables according to the ones   =%
%= set in configure.ac.                                                    =%

setlocal
set configuration=%1
if "%Configuration%"=="Debug" ( set Debug=DEBUG; ) else ( set Debug= )

setlocal DisableDelayedExpansion
mkdir Temp 2>nul

set conf=..\configure.ac
set tmp=Temp\version.txt

echo 1>&2 %~f0: Info: Replacing configuration variables

findstr "^LINK_.*_VERSION=" %conf% >  %tmp%
findstr "^VERSION="         %conf% >> %tmp%
for /F "tokens=*" %%i in (%tmp%) do set %%i
del %tmp%

	if "%Configuration%"=="" GOTO skipos
	%= Get OS name                                                         =%
	set "osreg=HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion"
	for /f "tokens=3*" %%i in ('reg query "%osreg%" /v ProductName') do (
			set "win_os= %%i %%j"
	)
	%= Get OS version                                                      =%
	for /f "tokens=3*" %%i in ('ver') do (
			set "win_ver= %%i %%j"
	)
:skipos

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

		if NOT "%Configuration%"=="" (
			set "line=!line:@HOST_OS@=%win_os% %win_ver%!"
			set "line=!line:@LG_DEFS@=%DEFS%!"
			set "line=!line:@CPPFLAGS@=%Debug%!"
			set "line=!line:@CFLAGS@=%CFLAGS%!"
		)

		echo(!line!
		endlocal
	)
)
