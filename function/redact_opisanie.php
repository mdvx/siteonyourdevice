<?php

 include_once 'mysql_connect.php';
 $W=mysqli_query($connect, "UPDATE users_site SET description='".$_POST['opisanie']."' where id_site='".$_POST['hidden']."'");
 echo "Описание сайта успешно изменено.";
 

?>