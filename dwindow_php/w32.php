<?php
date_default_timezone_set("PRC");
$date = Date("Y-m-d H:i:s");

// ip
$ip = "";
if(getenv('HTTP_CLIENT_IP'))
     $ip=getenv('HTTP_CLIENT_IP');
elseif(getenv('HTTP_X_FORWARDED_FOR'))
     $ip=getenv('HTTP_X_FORWARDED_FOR');
else
     $ip=getenv('REMOTE_ADDR');     

$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");
$RSA = false;
while (list($name, $value) = each($_GET))
{
	$RSA = $name;
}

if (!$RSA)
{
	die("ERROR: NO ARGUMENT");
}

$result = $com->decode_message($RSA);
if (!strpos($result, "R"))
{
	$paras = explode(",", $result);
	$passkey = $paras[0];
	$hash = $paras[1];
	$key = $paras[2];
}
else
{
	die($result);
}

$db = mysql_connect("localhost", "root", "tester88");
mysql_select_db("mydb", $db);

$result = mysql_query("INSERT INTO logs (ip, date, passkey, hash) values('".$ip."', '".$date."', '".$passkey."', '".$hash."');");

$result = mysql_query("SELECT * FROM active_passkeys where passkey = '".$passkey."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:INVALID PASSKEY");
	exit();
}

$result = mysql_query("SELECT pass FROM movies where hash='".$hash."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:HASH NOT FOUND");
	exit();
}
while ($myrow = mysql_fetch_row($result))
{
	printf("%s", $com->AES($myrow[0], $key));
}
?>