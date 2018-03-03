@echo off
echo ---+++--- Building Theora (Static) ---+++---

if .%SRCROOT%==. set SRCROOT=D:\xiph

set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include paths for Theora
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;%SRCROOT%\theora\include
echo Compiling...
msdev theora_static.dsp /useenv /make "theora_static - Win32 Release" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
