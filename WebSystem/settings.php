
<!DOCTYPE html>
<?php include 'base.php'; ?>

<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="index.php"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">System Settings</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">System Settings</h1>
        </div>
    </div><!--/.row-->
    <?php
        $config = 'cvfsweb.conf';
        $cvfs_dir = file_get_contents($config);
        $stripeconf = trim($cvfs_dir) . '/configs/stripe_size.conf';
        $cacheconf = trim($cvfs_dir) . '/configs/cache_size.conf';
        echo "configuration files = " . $stripeconf . '<br>' . $cacheconf . '<br>';
        
        if (isset($_POST['apply'])) {
            if (!empty($_POST['stripesz']) && !empty($_POST['cachesz'])) {
                file_put_contents($stripeconf, $_POST['stripesz']);
                file_put_contents($cacheconf, $_POST['cachesz']);
                echo "successfully changed configuration.";
            }
        }
        
        
        $stripesize = file_get_contents($stripeconf);
        $cachesize = file_get_contents($cacheconf);
        
        
        ?>
    <form action="settings.php" method="post">
        Stripe Size: <input name="stripesz" type="text" value="<?=$stripesize?>"> <br>
        Cache Size: <input name="cachesz" type="text" value="<?=$cachesize?>"> <br>
        <input type="submit" value="Apply" name="apply">
    </form>
</div>
</body>
</html>