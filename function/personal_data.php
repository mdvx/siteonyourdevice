<?php

$query = mysqli_query($connect, "SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
	$arr = mysqli_fetch_array ($query);
			echo "<table border=1 width=100%>
				<tr><td width=30%>Имя</td><td>".$arr["name"]."</td></tr>
				<tr><td width=30%>Фамилия</td><td>".$arr["second_name"]."</td></tr>
				<tr><td width=30%>E-mail</td><td>".$arr["email"]."</td></tr>
				<tr><td width=30%>Логин</td><td>".$arr["login"]."</td></tr>
				</table>";
	
	echo " 
	<fieldset style='position:absolute; bottom:10px; width:95%; border:none;';>
	<legend>Удаление аккаунта</legend>
	<form method=post action='account.php'>
	<table border=0 width=50% align=center>
	<tr><td> Вы действительно хотите удалить свой аккаунт? </td></tr>
	<tr><td><input class='form submit' type=submit name='deleteacc' value='Да, я хочу удалить аккаунт'></td></tr>
	</table>
	</form>
	</fieldset>";

?>