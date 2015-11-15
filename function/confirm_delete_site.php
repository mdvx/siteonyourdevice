<?php
	
	include_once 'mysql_connect.php';
	$W=mysqli_query($connect, "SELECT adds_site FROM users_site where id_site='".$_POST["hidden"]."' LIMIT 1");
	$results = mysqli_fetch_assoc($W);
echo " 
	<form method=post action='account.php'>
	<table border=0 width=50% align=center>
	<tr><td> Вы действительно хотите удалить сайт - ".$results['adds_site']."? </td></tr>
	<tr><td><input type=hidden name ='hidden' value='".$_POST["hidden"]."'></td></tr>
	<tr><td><input class='form submit' type=submit name='deletewebsite' value='Да, я хочу удалить сайт'>
		    <input class='form submit' type=submit name='registration_site' value='Нет, я не хочу удалять сайт'></td></tr>
	</table>
	</form>";
	
?>