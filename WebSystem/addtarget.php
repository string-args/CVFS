<!DOCTYPE html>
<html>
    <head>
        <meta charset="utf-8">
        <title>Add Target</title>
        <style>
            .details {
                margin-left: 50px;
            }
        </style>
    </head>
    <body>
        <h1>Add an ISCSI Target</h1>
        <form action="addt_serv.php" method="post">
            <input type="text" name="ipadd"> <!-- VERIFY IF PROPER IP FORMAT -->
            <input type="submit" value="Add">
        </form>
    </body>
</html>
