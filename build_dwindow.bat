copy/y dwindow\release_mt\dwindow.exe dwindow_NSIS
copy/y mySplitter\release_mt\mySplitter.ax dwindow_NSIS\codec
copy/y dwindow_launcher\release\*.exe dwindow_NSIS
copy/y SsifSource\bin\Filters_x86\MpegSplitter.ax dwindow_NSIS\codec\SsifSource.ax
pause
F:\NSIS\makensisw.exe dwindow_NSIS\dwindow.nsi