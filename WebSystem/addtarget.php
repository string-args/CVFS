<!DOCTYPE html>

<?php include 'base.php'; ?>


<div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
    <div class="row">
        <ol class="breadcrumb">
            <li><a href="index.php"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
            <li class="active">Add Target</li>
        </ol>
    </div><!--/.row-->

    <div class="row">
        <div class="col-lg-12">
            <h1 class="page-header">Add iSCSI Target</h1>
        </div>
    </div><!--/.row-->

    <form action="addt_serv.php" method="post">
        IP Address: <input type="text" name="ipadd"> <!-- VERIFY IF PROPER IP FORMAT -->
        <input type="submit" value="Add">
    </form>


</div>

</body>
</html>

