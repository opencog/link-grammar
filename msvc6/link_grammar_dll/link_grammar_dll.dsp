# Microsoft Developer Studio Project File - Name="link_grammar_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=link_grammar_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "link_grammar_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "link_grammar_dll.mak" CFG="link_grammar_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "link_grammar_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "link_grammar_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "link_grammar_dll - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINK_GRAMMAR_DLL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".." /I "..\.." /I "..\..\.." /I "F:\ETCETERA\JAVA\jdk1.6.0\include\\" /I "F:\ETCETERA\JAVA\jdk1.6.0\include\win32\\" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINK_GRAMMAR_DLL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib C:\opencog\lg_svn1\gnuwin32_regex\lib\regex.lib /nologo /dll /machine:I386 /out:"Release/link-grammar.dll"

!ELSEIF  "$(CFG)" == "link_grammar_dll - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINK_GRAMMAR_DLL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".." /I "..\.." /I "..\..\.." /I "F:\ETCETERA\JAVA\jdk1.6.0\include\\" /I "F:\ETCETERA\JAVA\jdk1.6.0\include\win32\\" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LINK_GRAMMAR_DLL_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib C:\opencog\lg_svn1\gnuwin32_regex\lib\regex.lib /nologo /dll /debug /machine:I386 /out:"Debug/link-grammar.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "link_grammar_dll - Win32 Release"
# Name "link_grammar_dll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\..\link-grammar\analyze-linkage.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\and.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\api.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\build-disjuncts.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\command-line.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\constituents.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\count.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\disjuncts.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\disjunct-utils.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\error.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\extract-links.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\fast-match.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\idiom.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\massage.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\post-process.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_knowledge.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_lexer.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\pp_linkset.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prefix.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\preparation.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print-util.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\print.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\prune.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\read-dict.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\read-regex.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\regex-morph.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\resources.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\spellcheck-hun.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\string-set.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\tokenize.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\utilities.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-file.c"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\word-utils.c"
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

SOURCE="..\..\link-grammar\api-types.h"
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

SOURCE="..\..\link-grammar\link-features.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\link-includes.h"
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

SOURCE="..\..\link-grammar\read-regex.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\regex-morph.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\gnuwin32_regex\include\regex.h
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\resources.h"
# End Source File
# Begin Source File

SOURCE="..\..\link-grammar\spellcheck-hun.h"
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
