<?php
$form = false;

include "db_and_basic_func.php";

while (list($name, $value) = each($_POST))
{
	if ($name == "username")
		$username = $value;
	if ($name == "password")
		$password = $value;
	if ($name == "admin")
		$admin = $value;
	if ($name == "op")
		$op = $value;
		
		$form = true;
}


if ($form)
{
	// check parameter
	if ($username == "")
		die("用户名为空");
		
	$username = str_replace("'", "''", $username);
	
	// test user
	$userexist = false;
	$result = mysql_query("SELECT * FROM users where name='".$username."'");
	if (mysql_num_rows($result) > 0)
		$userexist = true;
	
	// adding
	if ($op == "add")
	{
		if ($password == "")
			die("密码为空");

		if ($userexist)
		{
			printf("用户 '%s' 已存在.", $username);
			goto theend;
		}
		
		
		$pattern = "0123456789ABCDEF";
		$salt = "";
		for($i=0; $i < 64; $i++)
			$salt .= $pattern{mt_rand(0,15)};
		$result = mysql_query(sprintf("INSERT INTO users (name,pass_hash, usertype, bar_max_users, salt, deleted, expire) values ('%s', '%s'".
		", 0, 0, '%s', 0, %d)",$username, $com->SHA1($com->SHA1($password) . $salt), $salt, time()));
		if ($result)
		{
			printf("注册用户 %s 成功!", $username, $password);
			db_log("UserAdd", "OK", $username, $com->SHA1($password));
		}
		else
		{
			printf("adding user %s failed.", $username);
		}
	}
	
	// deleting
	if ($op == "del")
	{
		$string = mysql_query("INSERT INTO dropped_passkeys SELECT * FROM active_passkeys WHERE user='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}

		$result = mysql_query("DELETE FROM active_passkeys where user='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}
		$result = mysql_query("DELETE FROM users where name='".$username."'");
		if (!$result)
		{
			printf("delete user %s failed.");
			goto theend;
		}
		
		db_log("UserDel", $userexist ? "OK" : "NO USER", $username);
		printf("delete user %s, OK!", $username, $password);
	}
	
	// die
	theend:
	die("<BR>");
}

db_log("WWW", "OK", 0, "bomber.php");
?>
<html>
	<script language=javascript>
	function check(form)
	{
		if (form.username.value == "")
		{
			alert("用户名为空");
		}
		else if(form.password.value == form.password2.value)
		{
			form.submit();
		}		
		else
		{
			alert("密码不正确");
		}
	}
	</script>
	
	<form method="POST" name=form1>
		用户名       <input type="text" name="username" /> <br />
		密码        <input type="password" name="password" /> <br />
		重复密码        <input type="password" name="password2" /> <br />
		<input type="hidden" name = "op" value="add" />
		<input type="button" value="注册" onclick="check(this.form)"/>
	</form>
</html>