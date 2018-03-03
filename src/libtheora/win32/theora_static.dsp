# Microsoft Developer Studio Project File - Name="theora_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=theora_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "theora_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "theora_static.mak" CFG="theora_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "theora_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "theora_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "theora_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Static_Release"
# PROP Intermediate_Dir "Static_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\ogg\include" /I "..\..\theora\include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "theora_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Static_Debug"
# PROP Intermediate_Dir "Static_Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\ogg\include" /I "..\..\theora\include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Static_Debug\theora_static_d.lib"

!ENDIF 

# Begin Target

# Name "theora_static - Win32 Release"
# Name "theora_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lib\enc\dct.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\dct_decode.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\dct_encode.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\dct_encode.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encapiwrapper.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encode.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_huffman.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_idct.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_toplevel.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_quant.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\frarray.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\frinit.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\mathops.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\mcenc.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\mode.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\reconstruct.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\x86_32_vs\dsp_mmx.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\x86_32_vs\fdct_mmx.c
# End Source File
# Begin Source File

SOURCE=..\lib\enc\x86_32_vs\recon_mmx.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\apiwrapper.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\bitpack.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\decapiwrapper.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\decinfo.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\decode.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\dequant.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\fragment.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\huffdec.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\idct.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\info.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\internal.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\quant.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\state.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\x86_vc\mmxfrag.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\x86_vc\mmxidct.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\x86_vc\mmxloopfilter.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\x86_vc\mmxstate.c
# End Source File
# Begin Source File

SOURCE=..\lib\dec\x86_vc\x86stat.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\lib\dec\apiwrapper.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\block_inline.h
# End Source File
# Begin Source File

SOURCE=..\include\theora\codec.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\codec_internal.h
# End Source File
# Begin Source File

SOURCE=..\lib\cpu.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\dct.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\decint.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\dequant.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\dsp.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_huffman.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\encoder_lookup.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\enquant.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\huffdec.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\huffman.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\hufftables.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\idct.h
# End Source File
# Begin Source File

SOURCE=..\lib\internal.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\ocintrin.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\pp.h
# End Source File
# Begin Source File

SOURCE=..\lib\dec\quant.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\quant_lookup.h
# End Source File
# Begin Source File

SOURCE=..\include\theora\theora.h
# End Source File
# Begin Source File

SOURCE=..\include\theora\theoradec.h
# End Source File
# Begin Source File

SOURCE=..\lib\enc\toplevel_lookup.h
# End Source File
# End Group
# End Target
# End Project
