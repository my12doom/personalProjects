
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
// 4 = expired normal
$type2str = array("去广告", "7天", "1月", "1年");
$usertype2str = array("个人", "影院", "去广告", "网吧", "已到期的个人用户");
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
		die("无效的卡号或密码".$return_button);
	$row = mysql_fetch_array($result);
	if ($row["used"] != 0)
		die("此卡已被使用".$return_button);
	$cardtype = $row["type"];
		
			
		
	if ($usertype == 2 && $cardtype == 0)
	{
		printf("此用户已激活去广告，无需再使用去广告卡%s",$return_button);
		die();
	}
	if ($usertype == 0 && $expire > time() && $cardtype == 0)
	{
		printf("去广告卡无法用于已付费个人用户， 用户%s为%s用户%s",
					$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $usertype != 2 && $cardtype != 0)
	{
		printf("时间卡只能用于去广告用户转为个人用户，或用于个人用户增加时间， 用户%s为%s用户%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}
	else if ($usertype !=0 && $usertype != 2 && $cardtype == 0)
	{
		printf("去广告卡只能用于去广告， 特殊用户请使用专用的时间卡， 用户%s为%s用户%s",
				$username,$usertype2str[$usertype],$return_button);
		die();
	}		
	if($_POST["op"] == "valid")
	{
		// TODO
		die();
	}

?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name="op" value="post" />
		
		<?php

		// card type and time
		if ($cardtype == 0)
			echo("此卡为尚未使用的去广告卡<br>");
		else
			printf("此卡为尚未使用的时间卡（%s）<br>", $type2str[$cardtype]);

		printf("继续充值？<br>");
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
		卡号         <input type="text" name="card" /> <br />
		密码         <input type="text" name="password" /> <br />
		
		<input type="submit" value="充值！" onclick="this.form.submit()"/>
	</form>
</html>