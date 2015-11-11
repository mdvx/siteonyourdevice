<?php

$query = mysqli_query($connect, "SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
	$arr = mysqli_fetch_array ($query);
			echo "<table border=1 width=100%>
				<tr><td width=30%>Имя</td><td>".$arr["name"]."</td></tr>
				<tr><td width=30%>Фамилия</td><td>".$arr["second_name"]."</td></tr>
				<tr><td width=30%>E-mail</td><td>".$arr["email"]."</td></tr>
				<tr><td width=30%>Логин</td><td>".$arr["login"]."</td></tr>
				</table>";

?>