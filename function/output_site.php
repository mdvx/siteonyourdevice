<?php

    include_once 'mysql_connect.php';
	$W=mysqli_query($connect, "SELECT * FROM users_site where id_users='".$_COOKIE['id']."'");
	$num_results = mysqli_num_rows($W);
			if ($num_results!=0)
			{
			echo "<table border=1 width=100%>
				<tr><td width=30%>Адрес сайта</td>
			    <td width=70%>Описание</td></tr>";
			for ($n=0; $n<$num_results; $n++)
				{
				$arr = mysqli_fetch_array($W);
				echo "<tr><td>".$arr["adds_site"]."</td>
			    <td>".$arr["description"]."</td></tr>";
				} echo "</table>";
				
			}	

?>