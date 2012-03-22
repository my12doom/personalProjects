<?php
include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$user = $com->decode_binarystring($paras[0]);
	$password_hash = $paras[1];
	$return_key = $paras[2];
	$password_uncrypt = $com->decode_binarystring($paras[3]);
	$time = intval($paras[4]);
	$rev = intval($paras[5]);
}
else
{
	die($RSA_decoded);
}

// check time
if (time() - $time > 3600*12 || $time - time() > 3600*12)
{
	die("本机时间与服务器时间差异过大，请检查日期/时间设置");
}

// check hack
if (strstr($user, "'") || strstr($password_uncrypt, "'"))
{
	db_log("HACK:gen_key.php", "BLOCKED", str_replace("'", "''", $user), str_replace("'", "''", $password_uncrypt));
	die("ERROR:INVALID USERNAME OR PASSWORD");
}

// check rev
$rev_state = 1;
$result = mysql_query("SELECT * FROM revs where rev = ".$rev);
if (mysql_num_rows($result) > 0)
{
	$row = mysql_fetch_array($result);
	$rev_state = $row["state"];
}

if ($rev_state == 0)
{
	printf("此版本(rev%d)在服务器上被标记为尚未启用，请到\nhttp://bo3d.net/download下载最新版本", $rev);
	die("");
}
else if ($rev_state == 2)
{
	printf("此版本(rev%d)在服务器上被标记为已停用，请到\nhttp://bo3d.net/download下载最新版本", $rev);
	die("");
}

$passkey = $com->gen_freekey(time() - (12*3600), time() + 24*7*3600);
$passkey = $com->AES($passkey, $return_key);
echo $passkey;

db_log("ACTIVAT_FREE", "OK", $rev);

?>
