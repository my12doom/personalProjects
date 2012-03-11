<?php
die("")
$db = mysql_connect("localhost", "root", "");
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
	
$result = mysql_query("CREATE TABLE active_passkeys (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, passkey varchar(64), user varchar(32));", $db);
if (!$result)
	printf("CREATE TABLE active_passkeys FAILED<br>\n");

$result = mysql_query("CREATE TABLE dropped_passkeys (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY, passkey varchar(64), user varchar(32));", $db);
if (!$result)
	printf("CREATE TABLE dropped_passkeys FAILED<br>\n");
	
$result = mysql_query("CREATE TABLE logs (id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
											 ip varchar(15),
											 date varchar(40),
											 user varchar(64),
											 operation varchar(40),
											 result varchar(64),
											 arg1 varchar(64),
											 arg2 varchar(64),
											 arg3 varchar(64),
											 arg4 varchar(64),											 
											 reserved1 varchar(64));", $db);
if (!$result)
	printf("CREATE TABLE logs FAILED<br>\n");

$result = mysql_query("CREATE TABLE ips (
											 id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
											 start BIGINT,
											 end BIGINT,
											 address varchar(80));", $db);											 
if (!$result)
	printf("CREATE TABLE ips FAILED<br>\n");

$result = mysql_query("CREATE TABLE users 
											(id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
											name varchar(32), 
											pass_hash varchar(40), 
											passkey varchar(64));", $db);
if (!$result)
	printf("CREATE TABLE users FAILED<br>\n");


//$result = mysql_query("INSERT INTO users (passkey) VALUES ('user1')");
//if (!$result)
//	printf("INSERT user 1 FAILED<br>\n");

?>