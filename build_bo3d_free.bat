call revision_silent.bat

#build
set dev2003="C:\Program Files (x86)\Microsoft Visual Studio .NET 2003\Common7\IDE\devenv"
set dev2008="C:\Program Files (x86)\Microsoft Visual Studio 9.0\Common7\IDE\devenv"
cd /d "%~dp0"

#build
%dev2008% dwindow\dwindow.sln /build "Release_mt"
%dev2008% my12doomSource\my12doomSource.sln /build "Release Filter"
%dev2008% reset\reset.sln /build "Release"
%dev2008% report_server\report_server.sln /build "Release"

#copy
copy/y dwindow\dwindow.ini dwindow_NSIS
copy/y dwindow\alpha.raw dwindow_NSIS
copy/y dwindow\logo.raw dwindow_NSIS
copy/y dwindow\release_mt\StereoPlayer.exe dwindow_NSIS
copy/y reset\release\reset.exe dwindow_NSIS
copy/y report_server\release\ErrorReport.exe dwindow_NSIS
del/q dwindow_NSIS\codec\*.*
copy/y 3rdFilter\*.* dwindow_NSIS\codec
copy/y my12doomSource\bin\Filters_x86\*.ax dwindow_NSIS\codec

pause
C:\NSIS\makensisw.exe dwindow_NSIS\dwindow.nsi
cd dwindow_NSIS
set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%
del (free)bo3d%v%.exe
ren dwindow_setup.exe (free)bo3d%v%.exe
cd..
copy/y dwindow\release_mt\StereoPlayer.pdb pdbs\(free)bo3d%v%.pdb