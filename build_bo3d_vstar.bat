call revision_silent.bat

#build
set dev2003="C:\Program Files (x86)\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv"
set dev2008="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv"

#build
del dwindow\ico\ico.ico
copy dwindow\ico\icoVSTAR.ico dwindow\ico\ico.ico
%dev2008% dwindow\dwindow.sln /build "Release_vstar"
%dev2008% my12doomSource\my12doomSource.sln /build "Release Filter"
del dwindow\ico\ico.ico
copy dwindow\ico\ico_normal.ico dwindow\ico\ico.ico

#copy
copy/y dwindow\dwindow.ini dwindow_NSIS
copy/y dwindow\alpha.raw dwindow_NSIS
copy/y dwindow\logo.raw dwindow_NSIS
copy/y dwindow\Release_vstar\StereoPlayer.exe dwindow_NSIS
del/q dwindow_NSIS\codec\*.*
del/q dwindow_NSIS\skin\*.*
copy/y dwindow\skin\*.* dwindow_NSIS\skin
copy/y dwindow\xvidcore.dll dwindow_NSIS
copy/y 3rdFilter\*.* dwindow_NSIS\codec
copy/y mySplitter\release_mt\mySplitter.ax dwindow_NSIS\codec
copy/y my12doomSource\bin\Filters_x86\*.ax dwindow_NSIS\codec
pause
C:\NSIS\makensisw.exe dwindow_NSIS\dwindowVSTAR.nsi
cd dwindow_NSIS
set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%
del 3dvplayer%v%.exe
ren dwindow_setup.exe 3dvplayer%v%.exe