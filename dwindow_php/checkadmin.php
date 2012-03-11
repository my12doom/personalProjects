<?php
include "db_and_basic_func.php";

if (!isset($_COOKIE["dwindow_admin"]))
{	
	if (isset($_POST["admin_login_password"]))
	{
		if (!admin_pass($_POST["admin_login_password"]))
			die("wrong password");
		
		// generate a random session id
		$pattern = "0123456789ABCDEF";
		$session = "";
		for($i=0; $i < 64; $i++)
			$session .= $pattern{mt_rand(0,15)};
			
		setcookie("dwindow_admin", $session);
		$result = mysql_query("delete FROM admin_session;");
		$result = mysql_query(sprintf("insert into admin_session values (0, '%s')", $session));
		header('Location: admin.php');
	}
	else
	{
		header('Location: adminlogin.htm');
	}
}
else
{
	$session = $_COOKIE["dwindow_admin"];
}

// check session
$result = mysql_query(sprintf("select * from admin_session where session='%s'", $session));	// TODO: SQL injection
if (!$result)
{
		setcookie("dwindow_admin", $session, time()-3600);
		header('Location: adminlogin.htm');
}
if (mysql_num_rows($result) <= 0)
{
		setcookie("dwindow_admin", $session, time()-3600);
		header('Location: adminlogin.htm');
}
?>