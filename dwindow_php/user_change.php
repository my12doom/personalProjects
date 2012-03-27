
<?php
include "db_and_basic_func.php";
// card type:
// 0 = ad
// 1 = week
// 2 = month
// 3 = year

// user type:
// 0 = normal
// 1 = theater
// 2 = no ad
// 3 = bar

/*
              捐赠码0        个人码123
去广告2           x            o
影院/网吧13       x            x
已激活0+          x            x  
未激活0           O            O
*/

$type2str = array("去广告", "7天", "1月", "1年");
$usertype2str = array("个人", "影院", "捐赠", "网吧", "已到期的个人用户");
$return_button="<BR><input type='button' value='返回' onclick='history.go(-1)'/>";

if (isset($_POST["op"]))
{
	// check user and card first
	$username = str_replace("'", "''", $_POST["username"]);
	$card = str_replace("'", "''", $_POST["card"]);
	$password = str_replace("'", "''", $_POST["password"]);
		
	$result = mysql_query(sprintf("select * from users where name = '%s'", $username));
	if (mysql_num_rows($result) <= 0)
		die("用户 ". $username . " 未找到".$return_button);
	$row = mysql_fetch_array($result);
	$expire = $row["expire"];
	$usertype = $row["usertype"];

	$result = mysql_query(sprintf("select * from cards where code = '%s' and pass= '%s'",
						$card, $password));
	if (mysql_num_rows($result) <= 0)
		die("无效的激活码或密码".$return_button);
	$row = mysql_fetch_array($result);
	if ($row["used"] != 0)
		die("此激活码已被使用".$return_button);
	$cardtype = $row["type"];
		
			
		
	if ($usertype == 2 && $cardtype == 0)
	{
		printf("此用户已激活，无需再使用捐赠激活码%s",$return_button);
		die();
	}
	else if ($usertype == 2 && cardtype != 0)
	{
		//printf("此用户已激活，暂时无法使用个人激活码。%s",$return_button);
		//die();
	}
	else if ($usertype !=0 && $cardtype != 0)
	{
		printf("个人激活码只能用于个人用户激活， 用户%s为%s用户%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $cardtype == 0)
	{
		printf("捐赠激活码只能用于未激活账户， 特殊用户请使用专用的时间激活码， 用户%s为%s用户%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}		
	else if ($usertype == 0 && $expire > time() && $cardtype == 0)
	{
		printf("用户已激活， 用户%s为%s用户%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype == 0 && $expire > time() && $cardtype != 0)
	{
		printf("用户已激活， 用户%s为%s用户%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	// do it
	if($_POST["op"] == "valid")
	{
		// TODO
		$type2time = array(0xffffff, 7*3600*24, 31*3600*24, 366*3600*24);
		$extra_time = $type2time[$cardtype];
		$new_expire = 1931316605;//max($expire, time()) + $extra_time;
		$op_ok = true;
		
		if (/*$usertype == 0 && $expire < time()  &&*/ $cardtype == 0)
		{
			// 未激活用户，广告激活码
			$sql = sprintf("update users set usertype=2 where name='%s'", $username);
			$result = mysql_query($sql);
			//echo "1".mysql_error()."<BR>";
			$sql = sprintf("update users set expire=%d where name='%s'", $new_expire, $username);
			$result = mysql_query($sql);
			//echo "2".mysql_error()."<BR>";
			$sql = sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			//echo "3".mysql_error()."<BR>";
			db_log("USER_CHANGE", "OK", 0, $username, "捐赠", $card);
			
			/*
			$sql = sprintf("update users set usertype=2 where name='%s';", $username)
						.sprintf("update users set expire=%d where name='%s';", $new_expire, $username)
						.sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			echo "3".mysql_error()."<BR>";
			*/
		}
		else if (/*$usertype == 0 &&*/ $cardtype == 1)
		{
			// 个人激活码
			$sql = sprintf("update users set expire=%d where name='%s'", $new_expire, $username);
			$result = mysql_query($sql);
			//echo "4".mysql_error()."<BR>";
			$sql = sprintf("update cards set used = 1 where code = '%s'", $card);
			$result = mysql_query($sql);
			//echo "5".mysql_error()."<BR>";
			$sql = sprintf("update users set usertype=0 where name='%s'", $username);
			$result = mysql_query($sql);
			//echo "6".mysql_error()."<BR>";
			db_log("USER_CHANGE", "OK", 0, $username, "个人", $card);
		}
		else
		{
			db_log("USER_CHANGE", "FAIL", 0, $username, $card);
			die( "操作失败");
		}
		
		
		die("操作完成");
	}

?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		
		<?php

		// card type and time
		if ($cardtype == 0)
			echo("此激活码为尚未使用的捐赠激活码<br>");
		else
			printf("此激活码为尚未使用的个人激活码 %s<br>", "", $type2str[$cardtype]);

		printf("继续？<br>");
		?>
		
		<input type="hidden" name="op" value="valid" />
		<input type="hidden" name="username" value="<?php echo $_POST["username"] ?>"/> <br />
		<input type="hidden" name="card" value="<?php echo $_POST["card"] ?>" /> <br />
		<input type="hidden" name="password" value="<?php echo $_POST["password"] ?>"/> <br />
		<input type="submit" value="继续"/>
		<input type="button" value="返回" onclick="history.go(-1)"/>
	</form>
</html>

<?php
die();
}
?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		用户名       <input type="text" name="username" /> <br />
		激活码         <input type="text" name="card" /> <br />
		密码         <input type="text" name="password" /> <br />
		
		<input type="submit" value="充值！" onclick="this.form.submit()"/>
	</form>
</html>