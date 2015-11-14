<?php

  include_once 'mysql_connect.php';
	$W=mysqli_query($connect, "DELETE FROM users_site WHERE id_site='".$_POST['hidden']."' AND id_users='".$arr['id_users']."'");
		if ($W==true)
			{
			echo "Запись удалена";	
			}	
		else { echo "Что то пошло не так";}

?>