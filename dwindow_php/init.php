<?php
$db = mysql_connect("localhost", "root", "tester88");
$result = mysql_select_db("mydb");
if (!$db || !$result)
	die("connect failed<br>.\n");
	
//$result = mysql_query("DROP TABLE movies");
//$result = mysql_query("DROP TABLE users");
$result = mysql_query("CREATE TABLE movies (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, hash varchar(50), pass varchar(150));", $db);
if (!$result)
	printf("CREATE TABLE movies FAILED<br>\n");

//$result = mysql_query("INSERT INTO movies (hash, pass) VALUES ('hash1','pass1')");
//if (!$result)
//	printf("INSERT hash 1 FAILED<br>\n");
	
$result = mysql_query("CREATE TABLE users (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, passkey varchar(64));", $db);
if (!$result)
	printf("CREATE TABLE users FAILED<br>\n");

$result = mysql_query("CREATE TABLE logs (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, ip varchar(15), date varchar(40), passkey varchar(64), hash varchar(40));", $db);
if (!$result)
	printf("CREATE TABLE logs FAILED<br>\n");

//$result = mysql_query("INSERT INTO users (passkey) VALUES ('user1')");
//if (!$result)
//	printf("INSERT user 1 FAILED<br>\n");

?>