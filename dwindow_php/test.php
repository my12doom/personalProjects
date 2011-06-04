<?php
$db = mysql_connect("localhost", "root", "tester88");
mysql_select_db("mydb", $db);

$filehash = "";
$passkey = "";

while (list($name, $value) = each($_GET))
{
	$filehash = $name;
	$passkey = $value;
}

$result = mysql_query("SELECT * FROM users where passkey = '".$passkey."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:INVALID PASSKEY");
	exit();
}


$result = mysql_query("SELECT pass FROM movies where hash='".$filehash."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:HASH NOT FOUND");
	exit();
}
while ($myrow = mysql_fetch_row($result))
{
	printf($myrow[0]);
}

?>

