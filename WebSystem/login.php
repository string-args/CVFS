
<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Forms</title>

        <link href="bootstrap/css/bootstrap.min.css" rel="stylesheet">
        <link href="bootstrap/css/styles.css" rel="stylesheet">
    </head>

    <body>

        <?php
        session_start();

        if (isset($_SESSION['login'])) {
            header("location: index.php");
        }

        if (isset($_POST['submit'])) {
            if ($_POST['username'] == 'admin' && $_POST['password'] == 'admin') {
                $_SESSION['login'] = $_POST['username'];
                header("location: index.php");
            }
        }
        ?>

        <div class="row">
            <div class="col-xs-10 col-xs-offset-1 col-sm-8 col-sm-offset-2 col-md-4 col-md-offset-4">
                <div class="login-panel panel panel-default">
                    <div class="panel-heading">Log in</div>
                    <div class="panel-body">
                        <form role="form" action="login.php" method="POST">
                            <fieldset>
                                <div class="form-group">
                                    <input class="form-control" placeholder="username" name="username" type="text" autofocus="">
                                </div>
                                <div class="form-group">
                                    <input class="form-control" placeholder="password" name="password" type="password" value="">
                                </div>
                                <input type="submit" name="submit" value="Login" class="btn btn-primary">
                            </fieldset>
                        </form>
                    </div>
                </div>
            </div><!-- /.col-->
        </div><!-- /.row -->	



        <script src="bootstrap/js/jquery-1.11.1.min.js"></script>
        <script src="bootstrap/js/bootstrap.min.js"></script>
    </body>

</html>
