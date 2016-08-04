<!DOCTYPE html>

<?php
include 'base.php';
?>
<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="#"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">Icons</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">Dashboard</h1>
        </div>
    </div><!--/.row-->

    
    <div class="panel panel-primary col-md-6">
  <div class="panel-heading">
    <h2 class="panel-title">Active iSCSI Targets</h2>
  </div>
  <div class="panel-body">
    <?php

    function formatBytes($bytes, $precision = 2) {
        $units = array('B', 'KB', 'MB', 'GB', 'TB');

        $bytes = max($bytes, 0);
        $pow = floor(($bytes ? log($bytes) : 0) / log(1024));
        $pow = min($pow, count($units) - 1);

         $bytes /= pow(1024, $pow);

        return round($bytes, $precision) . ' ' . $units[$pow];
    }

    $config = 'cvfsweb.conf';
    $cvfs_dir = file_get_contents($config);
    $DBNAME = trim($cvfs_dir) . '/Database/cvfs_db';
//    echo "DBNAME = " . $DBNAME;
    $db = new SQLite3($DBNAME);
    $db->busyTimeout(5000);
//    $res = $db->query('SELECT * from Target;');
    $statement = $db->prepare('SELECT * FROM Target;');
    $res = $statement->execute();
    $total = 0;
    
    while ($row = $res->fetchArray()) {
        echo '<table class="table" border="2">';
        echo '<tr><td>Target IQN</td><td>' . $row['iqn'] . '</td></tr>';
        echo '<tr><td>IP Address</td><td>' . $row['ipadd'] . '</td></tr>';
        echo '<tr><td>Volume</td><td>' . $row['assocvol'] . '</td></tr>';
        echo '<tr><td>Mount Point</td><td>' . $row['mountpt'] . '</td></tr>';
        echo '<tr><td>Available Space (bytes)</td><td>' . $row['avspace'] . '</td></tr>';
        echo '</table><br>';
        $total += $row['avspace'];
    }


    $db->close();
    ?>
  </div>
</div>
    
    <div class="panel panel-primary col-md-6">
  <div class="panel-heading">
    <h2 class="panel-title">Total Available Space</h2>
  </div>
  <div class="panel-body">
    <h2><?=formatBytes($total)?></h2>
  </div>
</div>
    


</div>	<!--/.main-->

</body>
</html>
