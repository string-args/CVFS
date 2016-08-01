<!DOCTYPE HTML>

<?php
include 'base.php';
?>


<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="index.php"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">System Logs</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">System Logs</h1>
        </div>
    </div><!--/.row-->

    <?php
    $comm = "sudo cat /var/log/syslog | grep cvfs2";
    $status = exec($comm, $pout);
    echo "comm = " . $comm . '<br>';
    foreach ($pout as $l) {
        echo $l . '<br />';
    }
    ?>


</div>	<!--/.main-->
</body>
</html>
