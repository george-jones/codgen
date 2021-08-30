
call "C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\Tools\vsvars32.bat"

@IF "%1"=="" GOTO defaultmake

nmake -f Makefile.mak %1

@GOTO end

:defaultmake
nmake -f Makefile.mak

:end
