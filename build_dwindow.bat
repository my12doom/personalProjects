#build
set dev2003="C:\Program Files\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv"
set dev2008="C:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE\devenv"
E:
cd\
cd E:\private_projects
%dev2003% mySplitter\mySplitter.sln /build "Release_mt"
%dev2003% dwindow\dwindow.sln /build "Release_mt"
%dev2003% dwindow_launcher\dwindow_launcher.sln /build "Release"
%dev2008% SsifSource\SsifSource.sln /build "Release Filter"

#copy
copy/y dwindow\release_mt\dwindow.exe dwindow_NSIS
copy/y mySplitter\release_mt\mySplitter.ax dwindow_NSIS\codec
copy/y dwindow_launcher\release\*.exe dwindow_NSIS
copy/y SsifSource\bin\Filters_x86\MpegSplitter.ax dwindow_NSIS\codec\SsifSource.ax

pause
F:\NSIS\makensisw.exe dwindow_NSIS\dwindow.nsi
cd dwindow_NSIS
set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%
ren dwindow_setup.exe dwindow_setup%v%.exe