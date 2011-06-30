<?php
$form = false;
$com = new COM("phpcrypt.crypt") or die("failed loading cryptLib");

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
	if (strstr($username, "'"))
		die("''''''''''");
	if ($username == "")
		die("用户不能为空");
	if ($newpassword != $repeat)
		die("两次密码不同");

	$oldsha1 = $com->SHA1($password);
	$newsha1 = $com->SHA1($newpassword);

	$userexist = false;
	$db = mysql_connect("localhost", "root", "tester88");
	mysql_select_db("mydb", $db);
	$sql = sprintf("SELECT * FROM users where name='%s' and pass_hash='%s'", $username, $oldsha1);
	$result = mysql_query($sql);
	if (mysql_num_rows($result) <= 0)
		die("用户/ 密码错误");

	// deleting active passkey
	$result = mysql_query("DELETE FROM active_passkeys where user='".$username."'");
	if (!$result)
		goto theend;

	// update password
	$sql = sprintf("UPDATE users set pass_hash='%s' where name='%s'", $newsha1, $username);
	$result = mysql_query($sql);
	if (!$result)
		goto theend;

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