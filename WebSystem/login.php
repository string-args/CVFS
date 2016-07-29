<form action="login.php" method="POST">
    Username: <input type="text" name="username" /> <br />
    Password: <input type="text" name="password" />
    <input type="submit" value="Login" name="submit" />
</form>

<?php

session_start();

if(isset($_SESSION['login'])){
    header("location: index.php");
}

if(isset($_POST['submit'])) {
    if($_POST['username'] == 'admin' && $_POST['password'] == 'admin') {
        $_SESSION['login'] = $_POST['username'];
        header("location: index.php");
    }
}

?>