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

    <h2>Active iSCSI Targets</h2>
    <?php
    $config = 'cvfsweb.conf';
    $cvfs_dir = file_get_contents($config);
    $DBNAME = trim($cvfs_dir) . '/Database/cvfs_db';
    echo "DBNAME = " . $DBNAME;
    $db = new SQLite3($DBNAME);
    $db->busyTimeout(5000);
    $res = $db->query('SELECT * from Target;');
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

    echo '<h2>Total Available Space: ' . $total . ' bytes.</h2>';
    ?>

   
</div>	<!--/.main-->

</body>
</html>
