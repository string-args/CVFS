<!DOCTYPE HTML>
<html>
<body>
<?php

$ok = "SUCCESS";
$comm = "cat /var/log/samba-audit.log";
$status = exec($comm, $pout);

if ($status != $ok){
	foreach($pout as $l){
		echo $l . '<br />';
	}
} else {
	echo 'Log dont exist <br />';
}
echo '<a href="home.html"> HOME </a>';
?>
</body>
</html>
