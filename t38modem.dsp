# Microsoft Developer Studio Project File - Name="t38modem" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=t38modem - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "t38modem.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "t38modem.mak" CFG="t38modem - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "t38modem - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "t38modem - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "t38modem - Win32 No Trace" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 1
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "t38modem - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /c
# ADD CPP /nologo /MD /W4 /GR /GX /O2 /Ob2 /D "NDEBUG" /D "PTRACING" /Yu"ptlib.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 openh323.lib ptlib.lib comdlg32.lib winspool.lib wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib /nologo /subsystem:console /machine:I386

!ELSEIF  "$(CFG)" == "t38modem - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W4 /Gm /GR /GX /ZI /Od /D "_DEBUG" /D "PTRACING" /Yu"ptlib.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 openh323d.lib ptlibd.lib comdlg32.lib winspool.lib wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "t38modem - Win32 No Trace"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "NoTrace"
# PROP BASE Intermediate_Dir "NoTrace"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "NoTrace"
# PROP Intermediate_Dir "NoTrace"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W4 /GX /O2 /I "..\include" /D "NDEBUG" /Yu"ptlib.h" /FD /c
# ADD CPP /nologo /MD /W4 /GR /GX /O1 /Ob2 /D "NDEBUG" /D "PASN_NOPRINTON" /D "PASN_LEANANDMEAN" /Yu"ptlib.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0xc09 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ptlib.lib comdlg32.lib winspool.lib wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 openh323n.lib ptlib.lib comdlg32.lib winspool.lib wsock32.lib kernel32.lib user32.lib gdi32.lib shell32.lib advapi32.lib /nologo /subsystem:console /machine:I386

!ENDIF 

# Begin Target

# Name "t38modem - Win32 Release"
# Name "t38modem - Win32 Debug"
# Name "t38modem - Win32 No Trace"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\dle.cxx
# End Source File
# Begin Source File

SOURCE=.\drivers.cxx
# End Source File
# Begin Source File

SOURCE=.\drv_c0c.cxx
# End Source File
# Begin Source File

SOURCE=.\drv_pty.cxx
# End Source File
# Begin Source File

SOURCE=.\fcs.cxx
# End Source File
# Begin Source File

SOURCE=.\g7231_fake.cxx
# End Source File
# Begin Source File

SOURCE=.\hdlc.cxx
# End Source File
# Begin Source File

SOURCE=.\main.cxx
# End Source File
# Begin Source File

SOURCE=.\main_process.cxx
# End Source File
# Begin Source File

SOURCE=.\pmodem.cxx
# End Source File
# Begin Source File

SOURCE=.\pmodeme.cxx
# End Source File
# Begin Source File

SOURCE=.\pmodemi.cxx
# End Source File
# Begin Source File

SOURCE=.\pmutils.cxx
# End Source File
# Begin Source File

SOURCE=.\precompile.cxx
# ADD CPP /Yc"ptlib.h"
# End Source File
# Begin Source File

SOURCE=.\t30.cxx
# End Source File
# Begin Source File

SOURCE=.\t30tone.cxx
# End Source File
# Begin Source File

SOURCE=.\enginebase.cxx
# End Source File
# Begin Source File

SOURCE=.\t38engine.cxx
# End Source File
# Begin Source File

SOURCE=.\audio.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\dle.h
# End Source File
# Begin Source File

SOURCE=.\drivers.h
# End Source File
# Begin Source File

SOURCE=.\drv_c0c.h
# End Source File
# Begin Source File

SOURCE=.\drv_pty.h
# End Source File
# Begin Source File

SOURCE=.\fcs.h
# End Source File
# Begin Source File

SOURCE=.\g7231_fake.h
# End Source File
# Begin Source File

SOURCE=.\hdlc.h
# End Source File
# Begin Source File

SOURCE=.\main.h
# End Source File
# Begin Source File

SOURCE=.\pmodem.h
# End Source File
# Begin Source File

SOURCE=.\pmodeme.h
# End Source File
# Begin Source File

SOURCE=.\pmodemi.h
# End Source File
# Begin Source File

SOURCE=.\pmutils.h
# End Source File
# Begin Source File

SOURCE=.\t30.h
# End Source File
# Begin Source File

SOURCE=.\t30tone.h
# End Source File
# Begin Source File

SOURCE=.\enginebase.h
# End Source File
# Begin Source File

SOURCE=.\t38engine.h
# End Source File
# Begin Source File

SOURCE=.\audio.h
# End Source File
# Begin Source File

SOURCE=.\version.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
