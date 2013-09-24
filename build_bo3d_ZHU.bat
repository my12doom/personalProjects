call revision_silent.bat

#build
set dev2003="C:\Program Files (x86)\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv"
set dev2008="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv"
cd /d "%~dp0"

#build
%dev2008% mySplitter\mySplitter.sln /build "Release_mt"
%dev2008% dwindow\dwindow.sln /build "Release_zhu"
%dev2003% dwindow_launcher\dwindow_launcher.sln /build "Release"
%dev2003% pd10\pd10.sln /build "Release"
%dev2008% reset\reset.sln /build "Release"
%dev2008% ssifavs\ssifavs.sln /build "Release_mt"
%dev2008% coreavs\coreavs.sln /build "Release_mt"
%dev2008% MCavs\mcavs.sln /build "Release_mt"
%dev2008% SsifSource\SsifSource.sln /build "Release Filter"
%dev2008% 3dvSource\3dvSource.sln /build "Release"
%dev2008% my12doomSource\my12doomSource.sln /build "Release Filter"
%dev2008% report_server\report_server.sln /build "Release"
%dev2008% lua\lua.sln /build "Release"

#copy
copy/y dwindow\dwindow.ini dwindow_NSIS
copy/y dwindow\alpha.raw dwindow_NSIS
copy/y dwindow\logo.raw dwindow_NSIS
copy/y dwindow\Release_zhu\StereoPlayer.exe dwindow_NSIS
copy/y reset\Release\reset.exe dwindow_NSIS
copy/y lua\Release\lua.dll dwindow_NSIS
copy/y report_server\release\ErrorReport.exe dwindow_NSIS
del/q dwindow_NSIS\codec\*.*
del/q dwindow_NSIS\skin\*.*
copy/y dwindow\skin\*.* dwindow_NSIS\skin
copy/y dwindow\xvidcore.dll dwindow_NSIS
copy/y 3rdFilter\*.* dwindow_NSIS\codec
copy/y mySplitter\release_mt\mySplitter.ax dwindow_NSIS\codec
copy/y my12doomSource\bin\Filters_x86\E3DSource.ax dwindow_NSIS\codec\E3DSource.ax
copy/y my12doomSource\bin\Filters_x86\my12doomSource.ax dwindow_NSIS\codec\my12doomSource.ax
copy/y my12doomSource\bin\Filters_x86\MP4Splitter.ax dwindow_NSIS\codec\MP4Splitter.ax
rd/s/q dwindow_NSIS\UI
md dwindow_NSIS\UI
xcopy dwindow_UI\*.* dwindow_NSIS\UI /s /e /y
rd/s/q dwindow_NSIS\UI\3dvplayer
rd/s/q dwindow_NSIS\UI\DWindow2
for /r %%f in (dwindow_NSIS\UI\*.lua) do dwindow_NSIS\StereoPlayer.exe compile %%f %%f

#x264 tools
copy/y pd10\release\pd10.dll tools\
copy/y ssifavs\release_mt\ssifavs.dll tools\
copy/y coreavs\release_mt\coreavs.dll tools\
copy/y mcavs\release_mt\mcavs.dll tools\

pause
C:\NSIS\makensisw.exe dwindow_NSIS\dwindow.nsi
cd dwindow_NSIS
set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%
del (zhu)bo3d%v%.exe
ren dwindow_setup.exe (zhu)bo3d%v%.exe