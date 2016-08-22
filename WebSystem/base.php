
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>CVFS Web Admin Panel</title>

        <link href="bootstrap/css/bootstrap.min.css" rel="stylesheet">
        <link href="bootstrap/css/datepicker3.css" rel="stylesheet">
        <link href="bootstrap/css/styles.css" rel="stylesheet">

        <!--Icons-->
        <script src="bootstrap/js/lumino.glyphs.js"></script>
        <script src="bootstrap/js/jquery-1.11.1.min.js"></script>
        <script src="bootstrap/js/bootstrap.min.js"></script>
    </head>

    <body>
        <?php
        session_start();
        if (!isset($_SESSION['login'])) {
            header("location: login.php");
        }
        echo "Logged in as: " . $_SESSION['login'];
        ?>
        <nav class="navbar navbar-inverse navbar-fixed-top" role="navigation">
            <div class="container-fluid">
                <div class="navbar-header">
                    <button type="button" class="navbar-toggle collapsed" data-toggle="collapse" data-target="#sidebar-collapse">
                        <span class="sr-only">Toggle navigation</span>
                        <span class="icon-bar"></span>
                        <span class="icon-bar"></span>
                        <span class="icon-bar"></span>
                    </button>
                    <a class="navbar-brand" href="#"><span>CVFS2.0</span> Web Admin Panel</a>
                    <ul class="user-menu">
                        <li class="dropdown pull-right">
                            <a href="#" class="dropdown-toggle" data-toggle="dropdown"><svg class="glyph stroked male-user"><use xlink:href="#stroked-male-user"></use></svg> <?= $_SESSION['login'] ?> <span class="caret"></span></a>
                            <ul class="dropdown-menu" role="menu">
                                <li><a href="logout.php"><svg class="glyph stroked cancel"><use xlink:href="#stroked-cancel"></use></svg> Logout</a></li>
                            </ul>
                        </li>
                    </ul>
                </div>

            </div><!-- /.container-fluid -->
        </nav>

        <div id="sidebar-collapse" class="col-sm-3 col-lg-2 sidebar">
            <!-- class="glyphicons glyphicons-database-plus" glyphicons-332-dashboard.png -->
            <ul class="nav menu">
                <li><a href="index.php"><img src="glyphicons/png/glyphicons-332-dashboard.png" > Dashboard</a></li>
                <li><a href="systemlogs.php"><img src="glyphicons/png/glyphicons-88-log-book.png" > System Logs</a></li>
                <li><a href="auditlogs.php"><img src="glyphicons/png/glyphicons-72-book.png" > Audit Logs</a></li>
                <li><a href="addtarget.php"><img src="glyphicons/png/glyphicons-142-database-plus.png" > Add Target</a></li>
                <li><a href="decomnode.php"><img src="glyphicons/png/glyphicons-143-database-minus.png" > Decommission Server</a></li>
                <li><a href="settings.php"><img src="glyphicons/png/glyphicons-281-settings.png" > Settings</a></li>
                
                
                <li role="presentation" class="divider"></li>
                <li><a href="help.php"><img src="glyphicons/png/glyphicons-195-question-sign.png" > Help</a></li>
            </ul>

        </div><!--/.sidebar-->

        
        