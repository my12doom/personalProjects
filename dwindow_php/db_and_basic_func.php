<?php
$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");
if (!$com) die("500");

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
$RSA_decoded = "ERROR:NO ARGUMENT";
while (list($name, $value) = each($_GET))
	$RSA = $name;
if ($RSA)
	$RSA_decoded = $com->decode_message($RSA);

// db
$db = mysql_connect("localhost", "root", "tester88");
$result = mysql_select_db("mydb", $db);
if (!$db || !$result)
	die("500db");
	
function db_log($operation, $result, $user=0, $arg1=0, $arg2=0, $arg3=0, $arg4=0)
{
	global $ip;
	global $date;

	if (!$user || !is_string($user))
		$user = "unknown";
	if (!$operation || !is_string($operation))
		$operation = "unknown";
	if (!$result || !is_string($result))
		$result = "unknown";
	if (!$arg1 || !is_string($arg1))
		$arg1 = "";
	if (!$arg2 || !is_string($arg2))
		$arg2 = "";
	if (!$arg3 || !is_string($arg3))
		$arg3 = "";
	if (!$arg4 || !is_string($arg4))
		$arg4 = "";

	$sql = sprintf("INSERT INTO logs (ip, date, user, operation, result, arg1, arg2, arg3, arg4) values
																	 ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",
				 														$ip, $date, $user, $operation, $result, $arg1, $arg2, $arg3, $arg4);
	$result = mysql_query($sql);
}

function ip2address($ip)
{
	$out = "δ֪";
	$paras = explode(".", $ip);
	$a1 = intval($paras[0]);
	$a2 = intval($paras[1]);
	$a3 = intval($paras[2]);
	$a4 = intval($paras[3]);
	
	$strip = sprintf("0x%02x%02x%02x%02x", $a1, $a2, $a3, $a4);

	$sql = sprintf("SELECT * FROM ip_caches where start<=%s and %s <=end", $strip, $strip);
	$result = mysql_query($sql);
	if (mysql_num_rows($result) > 0)
	{
		$row = mysql_fetch_array($result);
		$out = $row["address"];
		return $out;
	}
	
	$sql = sprintf("INSERT INTO ip_caches SELECT * FROM IPS where start<=%s and %s <=end", $strip, $strip);
	$result = mysql_query($sql);
	$sql = sprintf("SELECT * FROM ip_caches where start<=%s and %s <=end", $strip, $strip);
	$result = mysql_query($sql);
	if (mysql_num_rows($result) > 0)
	{
		$row = mysql_fetch_array($result);
		$out = $row["address"];
	}
	
	return $out;	
}

function admin_pass($pass)
{
	global $com;
	$salt = "342A97D2DB6D1FBCDD7BD47393EFFD01A432609D4B006DB430F1FFB0644B39DD";
	$target_hash = "7DD3115A4A128BE0609F0400322705D564EDA03F";
	$hash = $com->SHA1($pass);
	$hash = $com->SHA1($hash.$salt);
	return $hash == $target_hash;
}

?>