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
	$user = $com->decode_binarystring($paras[0]);
	$password_hash = $paras[1];
	$return_key = $paras[2];
}
else
{
	die($result);
}

// search user
// test user : my12doom
// test key sha1:	0F4F2AA23F104C59F24BE531E3C1666B2169AA97	(tester88)
// $user = "my12doom";
// $password_hash = "0F4F2AA23F104C59F24BE531E3C1666B2169AA97";

$db = mysql_connect("localhost", "root", "tester88");
mysql_select_db("mydb", $db);

$result = mysql_query("SELECT * FROM users where name = '".$user."' and pass_hash = '".$password_hash."'");		//warning: possible SQL injection
if (mysql_num_rows($result) <= 0)
{
	$sql = sprintf("INSERT INTO logs (ip, date, passkey, hash, operation) values ('%s', '%s', '%s', '%s', '%s');", $ip, $date, $user, $password_hash, "ACTIVATION:INVALID USERNAME OR PASSWORD");
	$result = mysql_query($sql);
	die("ERROR:INVALID USERNAME OR PASSWORD");
}
$sql = sprintf("INSERT INTO logs (ip, date, passkey, hash, operation) values ('%s', '%s', '%s', '%s', '%s');", $ip, $date, $user, $password_hash, "ACTIVATION:OK");
$result = mysql_query($sql);


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
$passkey = $com->genkeys($passkey, time() - (12*7*3600), 0x7fffffff);
$passkey = $com->AES($passkey, $return_key);
echo $passkey;
?>