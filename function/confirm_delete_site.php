<?php

echo " 
	<form method=post action='account.php'>
	<table border=0 width=50% align=center>
	<tr><td> Вы действительно хотите удалить сайт? </td></tr>
	<tr><td><input type=hidden name ='hidden' value='".$_POST["hidden"]."'></td></tr>
	<tr><td><input class='form submit' type=submit name='deletewebsite' value='Да, я хочу удалить сайт'>
		    <input class='form submit' type=submit name='registration_site' value='Нет, я не хочу удалять сайт'></td></tr>
	</table>
	</form>";
	
?>