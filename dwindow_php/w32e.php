<?php

include "db_and_basic_func.php";

if (!strpos($RSA_decoded, "R"))
{
	$paras = explode(",", $RSA_decoded);
	$user = $com->decode_binarystring($paras[0]);
	$password_hash = $paras[1];
	$return_key = $paras[2];
	$requested_hash = $paras[3];
	$password_uncrypt = $com->decode_binarystring($paras[3]);
	$time = intval($paras[4]);
}
else
{
	die($RSA_decoded);
}


if (stripos($user, "login:") !== false)
{
	if ($password_hash != $admin_pass_sha1)
		die("ÃÜÂë´íÎó\n");
	else
		die("OK");
}
else if (stripos($user, "getkey:") !== false)
{
	$file = explode(":", $user);
	$file = $file[2];
	if ($password_hash != $admin_pass_sha1)
		die("ÃÜÂë´íÎó\n");
	else
	{
		// TODO
		$sql_string = sprintf("SELECT pass FROM movies where hash='%s'", $requested_hash);
		$result = mysql_query($sql_string);
		if (mysql_num_rows($result) <= 0)
		{
			// generate a random key and test if exist
			$pattern = "0123456789ABCDEF";
			$pass = "";
			retry:
			for($i=0; $i < 64; $i++)
				$pass .= $pattern{mt_rand(0,15)};
			
			// test exist
			$sql_string = sprintf("SELECT pass FROM movies where pass='%s'", $pass);
			$result = mysql_query($sql_string);
			if (mysql_num_rows($result) > 0)
				goto retry;
			
			// INSERT
			$sql_string = sprintf("INSERT INTO movies (hash,pass) values ('%s','%s')", $requested_hash, $pass);
			$result = mysql_query($sql_string);
			
			printf("OK\0");
			printf($com->AES($pass, $return_key));
		}
		else
		{
			printf("OK\0");
			if ($myrow = mysql_fetch_row($result))
			{
				printf($com->AES($myrow[0], $return_key));
			}
		}
		
		$sql_string = sprintf("SELECT * FROM movie_files where file='%s' and hash='%s'", $file, $requested_hash);
		$result = mysql_query($sql_string);
		if (mysql_num_rows($result) <= 0)
		{
			$sql_string = sprintf("INSERT INTO movie_files (hash,file) values ('%s','%s')", $requested_hash, $file);
			$result = mysql_query($sql_string);
		}

		die("");
	}
}
else
{
	die("Î´ÖªÖ¸Áî");
}
?>