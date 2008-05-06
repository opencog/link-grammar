# Microsoft Developer Studio Project File - Name="link grammar exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=link grammar exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "link grammar exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "link grammar exe.mak" CFG="link grammar exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "link grammar exe - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "link grammar exe - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "link grammar exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../.." /I "../../link-grammar" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../data/link-parser.exe"

!ELSEIF  "$(CFG)" == "link grammar exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../.." /I "../../link-grammar" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../data/link-parser.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "link grammar exe - Win32 Release"
# Name "link grammar exe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\link-grammar\link-parser.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\..\link-grammar\analyze-linkage.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\and.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\api-structures.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\api.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\build-disjuncts.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\command-line.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\constituents.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\count.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\error.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\externs.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\extract-links.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\fast-match.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\idiom.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\jni-client.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\link-features.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\link-includes.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\linkset.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\massage.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\post-process.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_knowledge.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_lexer.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_linkset.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prefix.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\preparation.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print-util.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prune.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\read-dict.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\resources.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\string-set.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\structures.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\tokenize.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\utilities.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-file.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-utils.h"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
# Microsoft Developer Studio Project File - Name="link grammar exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=link grammar exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "link grammar exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "link grammar exe.mak" CFG="link grammar exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "link grammar exe - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "link grammar exe - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "link grammar exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "../.." /I "../../link-grammar" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"../../data/link-parser.exe"

!ELSEIF  "$(CFG)" == "link grammar exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "../.." /I "../../link-grammar" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ  /c
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"../../data/link-parser.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "link grammar exe - Win32 Release"
# Name "link grammar exe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\link-grammar\link-parser.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\..\link-grammar\analyze-linkage.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\and.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\api-structures.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\api.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\build-disjuncts.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\command-line.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\constituents.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\count.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\error.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\externs.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\extract-links.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\fast-match.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\idiom.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\jni-client.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\link-features.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\link-includes.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\linkset.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\massage.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\post-process.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_knowledge.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_lexer.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_linkset.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prefix.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\preparation.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print-util.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prune.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\read-dict.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\resources.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\string-set.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\structures.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\tokenize.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\utilities.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-file.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-utils.h"
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
