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
$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");
if (!$com) die("500");
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

?>