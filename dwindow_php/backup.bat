set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.sql
set v2=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.rar
"C:\Program Files\MySQL\MySQL Server 5.5\bin\mysqldump.exe" -u root -ptester88 mydb >> %v%
rar a -ptester88 %v2% %v%
del %v%
copy %v2% F:\sqlbackup
move %v2% backup