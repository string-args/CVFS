<!DOCTYPE html>
<?php include 'base.php'; ?>

<style>
    .details {
                margin-left: 50px;
            }
</style>
<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="index.php"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">Server Decommissioning</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">Server Decommissioning</h1>
        </div>
    </div><!--/.row-->
    <h2>Select from available nodes</h2>
    <form action="decom_serv.php" method="post">
        <?php
        $config = 'cvfsweb.conf';
        $cvfs_dir = file_get_contents($config);
        $DBNAME = trim($cvfs_dir) . '/Database/cvfs_db';
        echo "DBNAME = " . $DBNAME . '<br>';
        $db = new SQLite3($DBNAME);
        $db->busyTimeout(5000);
        $res = $db->query('SELECT * from Target;');
        
        while ($row = $res->fetchArray()) {
            echo '<input type="radio" name="node" value="' . $row['iqn'] . '|' . $row['mountpt'] . '|' . $row['assocvol'] . '|' . $row['ipadd'] . '" />' . $row['iqn'] . '<br />';
            echo '<div class="details">' . 'IP address: ' . $row['ipadd'] . '<br />'
            . 'Volume: ' . $row['assocvol'] . '<br />'
            . 'Mount point: ' . $row['mountpt'] . '<br />'
            . 'Available Space: ' . $row['avspace'] . '<br />';
            echo '</div>';
        }
        $db->close();
        ?>
        

        <input type="submit" value="Decommission">
    </form>
</div>
</body>
</html>
