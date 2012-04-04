set v=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.sql
set v2=%DATE:~2,2%%DATE:~5,2%%DATE:~8,2%_%TIME:~0,2%%TIME:~3,2%%TIME:~6,2%.rar
"D:\Program Files\MySQL\MySQL Server 5.5\bin\mysqldump.exe" -u root -ptester88 mydb >>1.sql
rar a -ptester88 %v2% 1.sql
del 1.sql
copy %v2% G:\sqlbackup
move %v2% backup