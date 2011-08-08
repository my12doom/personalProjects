<?php

include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$user = $com->decode_binarystring($paras[0]);
	$password_hash = $paras[1];
	$return_key = $paras[2];
}
else
{
	die($RSA_decoded);
}

if (strstr($user, "'"))
{
	db_log("HACK:gen_key.php", "BLOCKED", str_replace("'", "''", $user), $password_hash);
	die("ERROR:INVALID USERNAME OR PASSWORD");
}

// search user
// test user : my12doom
// test key sha1:	0F4F2AA23F104C59F24BE531E3C1666B2169AA97	(tester88)
// $user = "my12doom";
// $password_hash = "0F4F2AA23F104C59F24BE531E3C1666B2169AA97";

$result = mysql_query("SELECT * FROM users where name = '".$user."' and pass_hash = '".$password_hash."'");		//warning: possible SQL injection
if (mysql_num_rows($result) <= 0)
{
	db_log("ACTIVE", "INVALID PASSWORD", $user, $password_hash);
	die("ERROR:INVALID USERNAME OR PASSWORD");
}
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
$passkey = $com->genkeys($passkey, time() - (12*3600), time() + 24*7*3600);
$passkey = $com->AES($passkey, $return_key);
echo $passkey;
?>