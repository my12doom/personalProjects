<?php

include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$passkey = $paras[0];
	$hash = $paras[1];
	$key = $paras[2];
}
else
{
	die($RSA_decoded);
}

$user = "unknown";
$result = mysql_query("SELECT * FROM active_passkeys where passkey = '".$passkey."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:INVALID PASSKEY");
	$result = mysql_query("SELECT * FROM dropped_passkeys where passkey = '".$passkey."'");
	if (mysql_num_rows($result) > 0)
	{
		$row = mysql_fetch_array($result);
		$user = "(del)".$row["user"];
	}
	db_log("E3D_REQUEST", "INVALID PASSKEY", $user, $passkey, $hash);
	exit();
}
else
{
		$row = mysql_fetch_array($result);
		$user = $row["user"];
}

$result = mysql_query("SELECT pass FROM movies where hash='".$hash."'");
if (mysql_num_rows($result) <= 0)
{
	printf("ERROR:HASH NOT FOUND");
	db_log("E3D_REQUEST", "HASH NOT FOUND", $user, $passkey, $hash);
	exit();
}
while ($myrow = mysql_fetch_row($result))
{
	printf("%s", $com->AES($myrow[0], $key));
}

db_log("E3D_REQUEST", "OK", $user, $passkey, $hash);
?>