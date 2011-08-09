<?php
include "db_and_basic_func.php";

$form = false;

while (list($name, $value) = each($_POST))
{
	if ($name == "password")
		$password = $value;
		$form = true;
}

if ($form)
{
	if ($password != "tester88")
		die("wrong password");
	
	$result = mysql_query("SELECT * FROM users order by id");
	while ($row = mysql_fetch_array($result))
	{		
		$user = $row["name"];
		printf("%d:%s<BR>\n", $row["id"], $user);
	}
	
	die("<BR>");
}
?>

<html>
	<form method="POST" name=form1>
		ÃÜÂë         <input type="password" name="password" /> <br />
		<input type="button" value="ÏÔÊ¾£¡" onclick="this.form.submit()"/>
	</form>
</html>