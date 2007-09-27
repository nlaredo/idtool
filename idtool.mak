# Microsoft Developer Studio Generated NMAKE File, Based on idtool.dsp
!IF "$(CFG)" == ""
CFG=idtool - Win32 Debug
!MESSAGE No configuration specified. Defaulting to idtool - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "idtool - Win32 Release" && "$(CFG)" != "idtool - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "idtool.mak" CFG="idtool - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "idtool - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "idtool - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "idtool - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\idtool.exe"


CLEAN :
	-@erase "$(INTDIR)\idtool.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\idtool.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O1 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\idtool.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\idtool.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib msvcrt.lib kernel32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\idtool.pdb" /machine:I386 /out:"$(OUTDIR)\idtool.exe" 
LINK32_OBJS= \
	"$(INTDIR)\idtool.obj"

"$(OUTDIR)\idtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "idtool - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\idtool.exe"


CLEAN :
	-@erase "$(INTDIR)\idtool.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\idtool.exe"
	-@erase "$(OUTDIR)\idtool.ilk"
	-@erase "$(OUTDIR)\idtool.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /Fp"$(INTDIR)\idtool.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\idtool.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=user32.lib msvcrt.lib kernel32.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\idtool.pdb" /debug /machine:I386 /out:"$(OUTDIR)\idtool.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\idtool.obj"

"$(OUTDIR)\idtool.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("idtool.dep")
!INCLUDE "idtool.dep"
!ELSE 
!MESSAGE Warning: cannot find "idtool.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "idtool - Win32 Release" || "$(CFG)" == "idtool - Win32 Debug"
SOURCE=.\idtool.c

"$(INTDIR)\idtool.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

