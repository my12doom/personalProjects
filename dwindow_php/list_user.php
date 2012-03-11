<?php
include "checkadmin.php";
	
$result = mysql_query("SELECT id,name FROM users order by id");
while ($row = mysql_fetch_array($result))
{		
	$user = $row["name"];
	printf("<A href = \"edituser.php?id=%s\">", $row["id"]);
	printf("%d:%s</a><BR>\n", $row["id"], $user);
}
	
?>