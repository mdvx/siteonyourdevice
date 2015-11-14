<?php

	$sait=clean($_POST['sait']); $opisanieSait=clean($_POST['opisanieSait']); 
		if ($sait!=null)
		{ 
		$query=mysqli_query($connect, "SELECT adds_site FROM users_site WHERE adds_site='".$sait."' ");
		$data = mysqli_fetch_array ($query);
			if ($data==null)
			{
			$s=mysqli_query($connect, "INSERT INTO users_site SET id_users='".$_POST['hidden']."', adds_site='".$sait."', description='".$opisanieSait."' ");
			echo "Сайт успешно зарегистрирован.";
			echo "<br>".mysql_error();
			//запись в базу redis
			$arr = array ('name' => $arr['name'], 'secondName' => $arr['second_name'], 'login' => $arr['login'], 'password'=>  $arr['password'], 'hosts' => array ('fastoredis.com:8080', 'fastonosql.com:8080', 'fastogt.com:8080')   );
			$jsonData = json_encode($arr);
			include 'redis_connect.php';
			$key = $arr['login'];
			$redis -> hset ('users',$key, $jsonData);
			}
			else { echo "Сайт с таким же доменным именем уже был зарегистрирован.";}
			
		}
		else 
		{ echo "Поле с доменным именем сайта пустым быть не должно.";}


function clean($value="")
{ $value = trim($value); $value=stripcslashes($value); $value=strip_tags($value); $value=htmlspecialchars($value); return $value; }		

?>