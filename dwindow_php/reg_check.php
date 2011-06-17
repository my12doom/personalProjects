<?php

// date
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

// decode RSA encrypted parameters
$RSA = false;
while (list($name, $value) = each($_GET))
	$RSA = $name;

if (!$RSA)
	die("ERROR: NO ARGUMENT");

$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");
$result = $com->decode_message($RSA);
if (!strpos($result, "R"))
{
	$paras = explode(",", $result);
	$passkey = $paras[0];
	//$hash = $paras[1];
	//$key = $paras[2];
}
else
{
	die($result);
}

// check passkey and return
$db = mysql_connect("localhost", "root", "tester88");
mysql_select_db("mydb", $db);

$sql = sprintf("INSERT INTO logs (ip, date, passkey, hash, operation) values ('%s', '%s', '%s', '%s', '%s');", $ip, $date, $passkey, "", "CHECK_REGISTER");
$result = mysql_query($sql);

$result = mysql_query("SELECT * FROM active_passkeys where passkey = '".$passkey."'");
if (mysql_num_rows($result) <= 0)
{
	$result = mysql_query("SELECT * FROM dropped_passkeys where passkey = '".$passkey."'");
	if (mysql_num_rows($result) > 0)
	{
		die("E_FAIL");
	}
	else
	{
		die("S_FALSE");
	}
}

die("S_OK");
?>