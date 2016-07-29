
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
                    <a class="navbar-brand" href="#"><span>CVFS2.0</span>Web Admin Panel</a>
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
            <form role="search">
                <div class="form-group">
                    <input type="text" class="form-control" placeholder="Search">
                </div>
            </form>
            <ul class="nav menu">
                <li><a href="index.php"><svg class="glyph stroked dashboard-dial"><use xlink:href="#stroked-dashboard-dial"></use></svg> Dashboard</a></li>
                <li><a href="systemlogs.php"><svg class="glyph stroked calendar"><use xlink:href="#stroked-calendar"></use></svg> System Logs</a></li>
                <li><a href="auditlogs.php"><svg class="glyph stroked line-graph"><use xlink:href="#stroked-line-graph"></use></svg> Audit Logs</a></li>
                <li><a href="addtarget.php"><svg class="glyph stroked table"><use xlink:href="#stroked-table"></use></svg> Add Target</a></li>
                <li><a href="decomnode.php"><svg class="glyph stroked table"><use xlink:href="#stroked-table"></use></svg> Decommission Server</a></li>
                <li><a href="help.php"><svg class="glyph stroked table"><use xlink:href="#stroked-table"></use></svg> Help</a></li>
                
                <li role="presentation" class="divider"></li>
                <li><a href="login.php"><svg class="glyph stroked male-user"><use xlink:href="#stroked-male-user"></use></svg> Login Page</a></li>
            </ul>

        </div><!--/.sidebar-->

        <div class="col-sm-9 col-sm-offset-3 col-lg-10 col-lg-offset-2 main">			
            <div class="row">
                <ol class="breadcrumb">
                    <li><a href="#"><svg class="glyph stroked home"><use xlink:href="#stroked-home"></use></svg></a></li>
                    <li class="active">Icons</li>
                </ol>
            </div><!--/.row-->

            <div class="row">
                <div class="col-lg-12">
                    <h1 class="page-header">Dashboard A</h1>
                </div>
            </div><!--/.row-->

            

            
        </div>	<!--/.main-->
        <script src="bootstrap/js/jquery-1.11.1.min.js"></script>
        <script src="bootstrap/js/bootstrap.min.js"></script>
    </body>