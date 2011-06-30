<?php
$form = false;
$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");

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
		die("empty username");
	if ($admin != "tester88")
		die("wrong admin password.");
	
	// test user
	$userexist = false;
	$db = mysql_connect("localhost", "root", "tester88");
	mysql_select_db("mydb", $db);
	$result = mysql_query("SELECT * FROM users where name='".$username."'");
	if (mysql_num_rows($result) > 0)
		$userexist = true;
	
	// adding
	if ($op == "add")
	{
		if ($password == "")
			die("empty password");

		if ($userexist)
		{
			printf("user %s already exist.", $username);
			goto theend;
		}
		
		$result = mysql_query("INSERT INTO users (name,pass_hash) values ('" . $username. "', '".$com->SHA1($password)."')");
		if ($result)
		{
			printf("adding user %s, password %s, OK!", $username, $password);
		}
		else
		{
			printf("adding user %s failed.");
		}
	}
	
	// deleting
	if ($op == "del")
	{
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
		
		printf("delete user %s, OK!", $username, $password);
	}
	
	// die
	theend:
	die("<BR>");
}
?>
<html>
	<form method="POST" name=form1>
		User Name       <input type="text" name="username" /> <br />
		Password        <input type="text" name="password" /> <br />
		Admin Password  <input type="password" name="admin" /> <br />
		<input type="hidden" name = "op" value="add" />
		<input type="button" value="Add User" onclick="this.form.op.value='add';this.form.submit()"/>
		<input type="button" value="Delete User" onclick="this.form.op.value='del';this.form.submit()"/>
	</form>
</html>