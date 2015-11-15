<?php

 include_once 'mysql_connect.php';
	
	$W=mysqli_query($connect, "DELETE FROM users WHERE id_users='".$arr['id_users']."'");
	$Q=mysqli_query($connect, "DELETE FROM users_site WHERE id_users='".$arr['id_users']."'");
		if ($W==true && $Q==true)
			{
			?><script type="text/javascript">alert("Спасибо что воспользовались нашим сервисом.");</script>
			<script type="text/javascript">location.href="index.php"</script><?php
			setcookie("id", "", time() - 3600*24*30*12);
			setcookie("hash", "", time() - 3600*24*30*12);
			}	
		else { echo "Что то пошло не так";}
	
?>