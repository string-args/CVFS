<?php
/*
    server side code of target add page
*/
$PROGNAME = './add_target';
$ok = 'SUCCESS';

$comm = $PROGNAME . ' ' . $_POST['ipadd'];
$status = exec($comm, $pout);
// we can print blah blah here, but we can really just redirect to fail / success page
// or even just redirect to homepage
if ($status != $ok) {
    echo 'Target Addition Failed <br />';
    foreach ($pout as $l) {
        echo $l . '<br />';
    }
} else {
    echo 'Target Addition Successful <br />';
}
echo '<a href="home.html">HOME</a>'
?>
