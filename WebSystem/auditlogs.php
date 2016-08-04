<!DOCTYPE HTML>

<?php
include 'base.php';
?>

<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="index.php"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">Audit Logs</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">Audit Logs</h1>
        </div>
    </div><!--/.row-->
    <?php
    $rows = "all";
    if (isset($_GET['logrow'])) {
        $rows = $_GET['logrow'];
    }
    ?>
    <div class="col-xs-3 ">

        <form method="get" name="chooselines">

            <select class="form-control" name="logrow" id="logrow" type="submit" onchange="document.getElementsByName('chooselines')[0].submit()">
                <option value="all" <?php if ($rows == "all"): echo "selected='selected'";
    endif;
    ?>>Display all rows</option>
                <option value="5" <?php if ($rows == "5"): echo "selected='selected'";
                        endif;
    ?>>Display last 5 rows</option>
                <option value="10" <?php if ($rows == "10"): echo "selected='selected'";
                        endif;
    ?>>Display last 10 rows</option>
                <option value="20" <?php if ($rows == "20"): echo "selected='selected'";
                        endif;
    ?>>Display last 20 rows</option>
            </select>
        </form>
    </div>
    <br>
    <?php
    $comm = "sudo cat /var/log/samba-audit.log";
    $status = exec($comm, $pout);

    echo "comm = " . $comm . '<br>';

    $pout = array_reverse($pout);
    if ($rows == "all")
        $rows = count($pout);
    $limit = 0;
    if (count($pout) < $rows)
        $limit = count($pout);
    else
        $limit = $rows;
    for ($index = 0; $index < $limit; $index++) {
        echo $pout[$index] . '<br />';
    }
    ?>


</div>
</body>
</html>