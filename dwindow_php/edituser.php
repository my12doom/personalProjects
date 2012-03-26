<?php
include "checkadmin.php";

if (!isset($_GET["id"]))
	die("no id");
	
$id = $_GET["id"];
$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
if (mysql_num_rows($result) <= 0)
	die("user not found");

$printinfo = true;
	
if (isset($_POST["op"]))
{
	$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	
	if ($_POST["op"] == "ban")
	{
		
		$result = mysql_query(sprintf("update users set deleted = 1-deleted where id = %s", $id));
		$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	}
	else if ($_POST["op"] == "del")
	{
		$result = mysql_query(sprintf("delete from users where id = %s", $id));
		die("user deleted, <a href=\"admin.php\"> click here to return.</a>");
		$printinfo = false;
	}
	else if ($_POST["op"] == "update")
	{
		$newdate = $_POST["newdate"];
		list($y, $m, $d) = sscanf($newdate, "%d-%d-%d");
		$newdate = mktime(0,0,0,$m,$d,$y);
		
		$result = mysql_query(sprintf("update users set expire = %s where id = %s", $newdate, $id));
		$result = mysql_query(sprintf("SELECT * FROM users where id = %s", $id));
	}
}


if ($printinfo)
{
	$row = mysql_fetch_array($result);
	printf("%s<BR>\n", $row["name"]);
	
	$usertype = $row["usertype"];
	printf("用户类型：%d(", $usertype);
	if ($usertype == 0)
		printf("普通用户");
	else if ($usertype == 1)
		printf("影院用户");
	else if ($usertype == 2)
		printf("去广告用户");
	else if ($usertype == 3)
	{
		printf("网吧用户");
		printf(", %d用户许可", $row["bar_max_users"]);
	}
	else
		printf("<font color=red>非法用户</font>");
			
	printf(")<br>\n");
			
	$date = Date("Y-m-d", $row["expire"]);
	

}

?>
<html>
	<script language=javascript>
	function warn_del(form)
	{
		if(confirm("are you sure want to delete this user? this operation is not revertable.") == 1)
		{
			form.op.value='del';
			form.submit();
		}
	}
	
	
	</script>
	<form method="POST" name=form1>
		<input type="hidden" name = "op" value="add" />
		到期日期：<input type="text" value="<?php echo $date?>" name="newdate"/>
		<input type="button" value="更新", onclick="this.form.op.value='update';this.form.submit()"/><p>
<?php
	if ($row["deleted"] != 0)
		printf("<font color=red> (已禁用)</font><p>\n");
?>
		<input type="button" value="禁用/解禁用户" onclick="this.form.op.value='ban';this.form.submit()"/>
		<input type="button" value="删除用户" onclick="warn_del(this.form)"/>
		<p>
		<p>
		<a href = "list_user.php"> 返回用户列表</a>
	</form>
</html>