nmake /? /c
if errorlevel 127 call "C:\Program Files\Microsoft Visual Studio 8\VC\bin\vcvars32.bat"
if errorlevel 127 call "C:\Program Files\Microsoft Visual Studio .NET\Vc7\bin\vcvars32.bat"
if errorlevel 127 call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"
call "C:\Program Files\Microsoft Platform SDK\SetEnv.cmd" /DEBUG
if errorlevel 127 call "C:\Program Files\Microsoft SDK\SetEnv.Bat" /DEBUG
nmake /a /f idtool.mak CFG="idtool - Win32 Debug"
call "C:\Program Files\Microsoft Platform SDK\SetEnv.cmd" /RETAIL
if errorlevel 127 call "C:\Program Files\Microsoft SDK\SetEnv.Bat" /RETAIL
nmake /a /f idtool.mak CFG="idtool - Win32 Release"
