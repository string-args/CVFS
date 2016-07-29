<!DOCTYPE HTML>
<html>
<body>
<?php

include 'base.php';

$comm = "sudo cat /var/log/syslog | grep cvfs2";
$status = exec($comm, $pout);

foreach($pout as $l){
	echo $l . '<br />';
}
echo '<a href="home.html"> HOME </a>';
?>
</body>
</html>
