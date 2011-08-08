<?php

include "db_and_basic_func.php";

$form = false;

while (list($name, $value) = each($_POST))
{
	if ($name == "username")
		$username = $value;
	if ($name == "password")
		$password = $value;
	if ($name == "newpassword")
		$newpassword = $value;
	if ($name == "repeat")
		$repeat = $value;

		$form = true;
}

if ($form)
{
	// check parameter
	if (strstr($username, "'") or strstr($newpassword, "'") or strstr($password, "'"))
	{
		$username = str_replace("'", "''", $username);
		$password = str_replace("'", "''", $password);
		$newpassword = str_replace("'", "''", $newpassword);
		
		db_log("HACK:user.php", "BLOCKED", $username, $password, $newpassword);
		die("''''''''''");
	}
	if ($username == "")
		die("用户不能为空");
	if ($newpassword != $repeat)
		die("两次密码不同");

	$oldsha1 = $com->SHA1($password);
	$newsha1 = $com->SHA1($newpassword);

	$userexist = false;
	$sql = sprintf("SELECT * FROM users where name='%s' and pass_hash='%s'", $username, $oldsha1);
	$result = mysql_query($sql);
	if (mysql_num_rows($result) <= 0)
	{
		db_log("PasswordCrack", "BLOCKED", $username, $password, $newpassword);
		die("用户/ 密码错误");
	}

	// deleting active passkey
	$result = mysql_query("DELETE FROM active_passkeys where user='".$username."'");
	if (!$result)
		goto theend;

	// update password
	$sql = sprintf("UPDATE users set pass_hash='%s' where name='%s'", $newsha1, $username);
	$result = mysql_query($sql);
	if (!$result)
		goto theend;

	db_log("PasswordChange", "OK", $username, $oldsha1, $newsha1);
	printf("成功, %s的密码已修改，请重新激活。", $username);

	// die
	theend:
	die("<BR>");
}
?>

<html>
	<form method="POST" name=form1>
		用户名       <input type="text" name="username" /> <br />
		密码         <input type="password" name="password" /> <br />
		新密码       <input type="password" name="newpassword" /> <br />
		确认密码     <input type="password" name="repeat" /> <br />
		<input type="button" value="修改！" onclick="this.form.submit()"/>
	</form>
</html>