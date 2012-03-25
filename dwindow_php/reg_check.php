<?php

include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$passkey = $paras[0];
	//$hash = $paras[1];
	$return_key = $paras[2];
	$time = intval($paras[4]);
	$rev = intval($paras[5]);
}
else
{
	die("E_FAIL:".$result);
}

// check time
if (time() - $time > 3600*12 || $time - time() > 3600*12)
{
	//die("S_OK\0本机时间与服务器时间差异过大，请检查日期/时间设置");
}

// check trial
if ($rev == 2)
{
$passkey = $com->gen_freekey(time() - (12*3600), time() + 24*7*3600);
$passkey = $com->AES($passkey, $return_key);
db_log("PLAYER_STARTUP_FREE", "S_OK", $rev);
echo "S_OK\0";
echo $passkey;
die("\0free");
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
	printf("E_FAIL\0此版本(rev%d)在服务器上被标记为尚未启用，请到\nhttp://bo3d.net/download下载最新版本", $rev);
	die("");
}
else if ($rev_state == 2)
{
	printf("E_FAIL\0此版本(rev%d)在服务器上被标记为已停用，请到\nhttp://bo3d.net/download下载最新版本", $rev);
	die("");
}

// check passkey and return
$sql = sprintf("INSERT INTO logs (ip, date, passkey, hash, operation) values ('%s', '%s', '%s', '%s', '%s');", $ip, $date, $passkey, "", "CHECK_REGISTER");
$result = mysql_query($sql);

$user = "unknown";
$rtn = "S_FALSE";
$result = mysql_query("SELECT * FROM active_passkeys where passkey = '".$passkey."'");
$newkey = false;
if (mysql_num_rows($result) <= 0)
{
	$result = mysql_query("SELECT * FROM dropped_passkeys where passkey = '".$passkey."'");
	if (mysql_num_rows($result) > 0)
	{
		$row = mysql_fetch_array($result);
		$user = "(del)".$row["user"];
		
		$rtn = "E_FAIL";
	}
}
else
{
	$row = mysql_fetch_array($result);
	$user = $row["user"];
	$rtn = "S_OK\0";
	
	// get max_bar_user and usertype
	$result = mysql_query("SELECT * FROM users where name = '".$user."'");
	if (mysql_num_rows($result) <= 0)
	{
		db_log("HACK_ACTIVE_PASSKEY", $user, $passkey);
		die("E_FAIL");
	}
	
	// passkey update
	$row = mysql_fetch_array($result);
	$bar_user = $row["bar_max_users"];
	$theater = $row["usertype"];
	$expire = $row["expire"];
	$newkey = $com->genkey4($passkey, time() - (12*3600), min($expire, time() + 24*7*3600), $bar_user, $theater);
	$newkey = $com->AES($newkey, $return_key);

}

db_log("PlayerStartup", $rtn, $user, $passkey);
echo $rtn;
if ($newkey)echo $newkey;
?>