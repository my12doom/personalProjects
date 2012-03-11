<?php
$t = "http://dwindow.bo3d.net/gen_key.php";
$p = "";
while (list($name, $value) = each($_GET))
	$p = $name;
header("Location: ".$t."?".$p);
?>