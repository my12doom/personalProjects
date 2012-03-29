<?php
include "checkadmin.php";
db_log("WWW", "OK", 0, $_SERVER['PHP_SELF']);

/*CREATE TABLE cards (id INT AUTO_INCREMENT PRIMARY KEY,
											used INT, 
											type INT, 
											time INT, 
											code char(40), 
											pass char(40))
*/

if (isset($_POST["op"]) && $_POST["op"] == "gen")
{
	if (!isset($_POST["cardtype"]))
		die("请选择一种卡类型");
		
	//card type:
	// 0 = ad
	// 1 = week
	// 2 = month
	// 3 = year
	
$type = $_POST["cardtype"];
$pattern = "0123456789ABCDEF";
$type2str = array("jz", "gr");
for($t=0; $t<$_POST["number"]; $t++)
{
	$code = "";
	$pass = "";
	for($i=0; $i < 15; $i++)
	{
		$code .= $pattern{mt_rand(0,9)};
		$pass .= $pattern{mt_rand(0,9)};
	}
	//printf("%s卡:%s - %s <br>\n", $type2str[$type], $code, $pass);
	printf("%s,%s,%s<br>\n", $code, $pass, $type2str[$type]);

	$sql = sprintf("insert into cards (used, type, time, code, pass) values"
								."(0, %d, 0, '%s', '%s')", $type, $code, $pass);
	$result = mysql_query($sql);
}
db_log("GEN_CODE", "OK", 0, $type2str[$type], $_POST["number"]);
die();
}
?>

<html>
	<form method="POST" name=form1>
		<input type="hidden" name = "op" value="gen" />
		<input type="radio" name="cardtype" value="0"> 捐赠<br>
		<input type="radio" name="cardtype" value="1"> 个人 <br>
		生成数量 <input type="text" name="number" value=10><br>
		<input type="submit" value="生成！">
	</form>
</html>