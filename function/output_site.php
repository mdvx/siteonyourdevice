<?php

    include_once 'mysql_connect.php';
	$W=mysqli_query($connect, "SELECT * FROM users_site where id_users='".$_COOKIE['id']."'");
	$num_results = mysqli_num_rows($W);
			if ($num_results!=0)
			{
			echo " <form method='post' action='account.php'>
				<table border=1 rules='rows' width=100%>
				<tr><td width=30%>Адрес сайта</td> <td width=70% colspan=2>Описание</td></tr>";
			for ($n=0; $n<$num_results; $n++)
				{
				$arr = mysqli_fetch_array($W);
				echo "<tr><td>".$arr["adds_site"]."</td> 
				<td><textarea name='opisanie' cols=50 rows = 3>".$arr["description"]."</textarea></td>
				<td> <input type=hidden name ='hidden' value='".$arr["id_site"]."'> 
						<input type=submit name='redact_opisanie' value='Сохранить описание'>
						<input type=submit name='delete_site' value='Удалить сайт'></td> </tr>";
				} echo "</table> </form>";
				
			}	

?>