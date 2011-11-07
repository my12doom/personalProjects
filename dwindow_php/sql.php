<?php
include "db_and_basic_func.php";
db_log("WWW", "OK", 0, $_SERVER['PHP_SELF']);

$form = false;

while (list($name, $value) = each($_POST))
{
	if ($name == "password")
		$password = $value;
	if ($name == "sql")
		$sql = $value;
	$form = true;
}

if ($form)
{
	if ($password != $admin_pass)
	{
		db_log("SQL", "wrong password", 0, str_replace("'", "''", $password), str_replace("'", "''", $sql));
		die("wrong password");
	}
	
	printf("sql = %s.<br>\n", $sql);
	$result = mysql_query($sql);
	if (!$result)
		die(mysql_error());
	
	db_log("SQL", "OK", 0, "hidden", str_replace("'", "''", $sql));
	
	$first = true;
	echo("<table border=3>\n");
	while ($row = mysql_fetch_array($result))
	{
		if ($first)
		{
			echo("<tr>\n");
			$firstrow = $row;
			while (list($name, $value) = each($firstrow))
			{
				if (!Is_Numeric($name))
					printf("<th>%s</th>", $name);
			}
			$first = false;
			echo("</tr>\n");
		}
			
		{
			echo("<tr>\n");
			while (list($name, $value) = each($row))
			{
				if (!Is_Numeric($name))
				{
					if ($name == "ip")
						printf("<td>%s(%s)</td>", $value, ip2address($value));
					else
						printf("<td>%s</td>", $value);
				}
			}
			echo("</tr>\n");
		}
		
	}
	echo("</table>");
	die("");
}
?>



<html>
	<form method="POST" name=form1>
		√‹¬Î         <input type="password" name="password" /> <br />
		SQL         <input type="text" name="sql" /> <br />
		<input type="button" value="œ‘ æ£°" onclick="this.form.submit()"/>
	</form>
</html>