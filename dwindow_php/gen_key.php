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


// search user
// test user : my12doom
// test key sha1:	0F4F2AA23F104C59F24BE531E3C1666B2169AA97	()
// $user = "my12doom";
// $password_hash = "0F4F2AA23F104C59F24BE531E3C1666B2169AA97";

$max_bar_user = 0;
$usertype = 0;
$result = mysql_query("SELECT * FROM users where name = '".$user."'");		//warning: possible SQL injection
if (mysql_num_rows($result) <= 0)
{
	db_log("ACTIVE", "NO such USER", $user, $password_hash, $password_uncrypt);
	die("ERROR:INVALID USERNAME OR PASSWORD, 无效的用户名或密码");	
}
$row = mysql_fetch_array($result);
$salt = $row["salt"];
$pass_hash = $row["pass_hash"];
if ($com->SHA1($password_hash.$salt) != $pass_hash)
{
	db_log("ACTIVE", "INVALID PASSWORD", $user, $password_hash, $password_uncrypt);
	die("ERROR:INVALID USERNAME OR PASSWORD, 无效的用户名或密码");
}

$expire = $row["expire"];
if ($expire <= time())
{
	db_log("ACTIVE", "EXPIRED", $user, $password_hash, $password_uncrypt);
	die("ERROR:USER EXPIRED, 用户已过期");
}

if ($row["deleted"] != 0)
{
	db_log("ACTIVE", "BANNED", $user, $password_hash, $password_uncrypt);
	die("ERROR:USER BANNED, 用户已被禁用");
}

$max_bar_user = $row["bar_max_users"];
$usertype = $row["usertype"];
db_log("ACTIVE", "OK", $user, $password_hash);

// generate a random passkey and test if in dropped_passkey table
retry:
$pattern = "0123456789ABCDEF";
$passkey = "";
for($i=0; $i < 64; $i++)
	$passkey .= $pattern{mt_rand(0,15)};

	// test dropped table
$result = mysql_query("select * FROM dropped_passkeys WHERE passkey='".$passkey."'");
if (mysql_num_rows($result) > 0)
	goto retry;
	
// delete from active passkeys to dropped and recreate one
$string = "INSERT INTO dropped_passkeys SELECT * FROM active_passkeys WHERE user='".$user."'";
$result = mysql_query($string);
$result = mysql_query("delete FROM active_passkeys WHERE user='".$user."'");
$result = mysql_query("INSERT INTO active_passkeys (passkey, user) values('".$passkey."', '".$user."')");

// encode them to RSA activation code
$passkey = $com->genkey4($passkey, time() - (12*3600), min(time() + 24*7*3600, $expire), $max_bar_user, $usertype);
$passkey = $com->AES($passkey, $return_key);
echo $passkey;
?>