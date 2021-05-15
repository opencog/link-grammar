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

findstr "^LINK_.*_VERSION="     %conf% >  %tmp%
findstr "^VERSION="             %conf% >> %tmp%
findstr "^DISCUSSION_GROUP="    %conf% >> %tmp%
findstr "^OVERVIEW="            %conf% >> %tmp%

set count=0

if "%Configuration%"=="" GOTO endconf

REM Input format:
REM # Optional comments
REM AC_INIT([link-grammar],[5.5.1],[https://github.com/opencog/link-grammar], ,
REM         [https://www.abisource.com/projects/link-grammar/])
REM <BLANK LINE>
REM
REM findstr: Generate line numbers (/n) to detect empty line,
REM and skip comments. Use tokens and delims to skip line number field.
REM Use "$" to preserve empty comma-delimited fields. Remove "[])".
REM XXX "!" is not preserved so it should not appear in the input!
for /f "tokens=1* delims=:" %%i in ('findstr /n  /v "^\s*#" %conf%') do (
	if "%%j"=="" GOTO endconf
	set "_line=%%j"
	setlocal EnableDelayedExpansion
	set "_line=!_line:,=$,!"
	set "_line=!_line:)=$)!"
	for %%y in (!_line!) do (
		for %%x in (%%y) do (
			set "_t=%%x"
			set "_t=!_t:)=!"
			set "_t=!_t:~0,-1!"
			set "_t=!_t:[=!"
			set "_t=!_t:]=!"
			set "x!count!=!_t!"
			set /a count+=1
		)
	)
)
:endconf
set "PACKAGE_BUGREPORT=%x2%"
set "PACKAGE_URL=%x4%"
REM set PACKAGE_BUGREPORT 1>&2
REM set PACKAGE_URL 1>&2

setlocal DisableDelayedExpansion
for /F "tokens=*" %%i in (%tmp%) do set %%i
del %tmp%

REM set DISCUSSION_GROUP 1>&2
REM set OVERVIEW 1>&2
REM set LINK_ 1>&2
REM set VERSION 1>&2
REM echo END configuration variables 1>&2

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

%= Process the ".in" file, preserving blank lines and exclamation marks =%
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

			set "line=!line:@DISCUSSION_GROUP@=%DISCUSSION_GROUP%!"
			set "line=!line:@OVERVIEW@=%OVERVIEW%!"
			set "line=!line:@PACKAGE_BUGREPORT@=%PACKAGE_BUGREPORT%!"
			set "line=!line:@PACKAGE_URL@=%PACKAGE_URL%!"
			set "line=!line:'=!"
		)

		echo(!line!
		endlocal
	)
)
:end
