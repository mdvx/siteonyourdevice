<?php  if (!isset($_COOKIE['id']) & !isset($_COOKIE['hash'])) 
	{ 		
		?><script type="text/javascript">alert("Авторизируйтесь пожалуйста");</script>
		<script type="text/javascript">location.href="index.php"</script><?php
	}
	if ( isset($_POST['exit']) ) 
	{ 		
		setcookie("id", "", time() - 3600*24*30*12);
		setcookie("hash", "", time() - 3600*24*30*12);
		?><script type="text/javascript">location.href="index.php"</script><?php
	}
?>
<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Тест</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style.css">

</head>
 <body>
<div class="context header borderleft borderright">
 <div class="head"> <div class="left">Региcтрация сайта  </div></div>
 <div class="head">
 <?php
	echo "
	<div class='right'><form method=post action='account.php'>
	<input class='form submit' type=submit name='exit' value='Выйти из аккаунта'>
	</form> </div>  
	";
 ?>
 </div>
</div>
<!--меню -->
<div class="context bordertop borderleft borderright borderbottom">
<div style="width:100%; height:0px; clear:both;"></div>
	<form method=post action='account.php'><input type=submit class="button button1" name="private_data" value="Личные данные"></form>
	<form method=post action='account.php'><input type=submit class="button button2" name="registration_data" value="Регистрация сайта"></form>
	<form method=post action='account.php'><input type=submit class="button button3" name="registration_site" value="Зарегистрированные сайты"></form>
<div style="width:100%; height:0px; clear:both;"></div>
</div>
   
 <div class="context middle borderleft borderright">
<?php
include_once 'mysql_connect.php'; 

$query = mysqli_query ($connect, "SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
$arr = mysqli_fetch_assoc($query);


if ( isset($_POST['private_data']) ) { include_once 'personal_data.php'; } //данные о пользователе

if ( isset($_POST['registration_data']) | isset($_POST['ok']) )
{
echo " 
<form method=post action='account.php'>
<table border=0>
<tr><td> Доменное имя сайта </td><td><input class='form' type=text name='sait'></td></tr>
<tr><td> Описание сайта </td><td><textarea class='form' rows=5  name='opisanieSait'></textarea></td></tr>
<tr><td> </td> <td><input type=hidden name='hidden' value='".$arr['id_users']."'></td></tr>
<tr><td><input class='form submit' type=submit name='okregistr' value='Зарегистрировать'></td></tr>
</table>
</form>";
}

//регистрация сайта
if (isset($_POST['okregistr'])) 
{ 	include_once 'registration_site.php'; 

/*if ($_POST['sait']!=null) 
	{
		//запись в базу редис
		
		$arr = array ('name' => $arr['name'], 'secondName' => $arr['second_name'], 'login' => $arr['login'], 'password'=>  $arr['password'], 'sait' => $_POST['sait'], 'hosts' => array ('127.0.0.1', '127.0.0.2', '127.0.0.3')   );
		$jsonData = json_encode($arr);
		//include 'redis_connect.php';
		$redis = new Redis(); 
		$redis->connect('127.0.0.1'); 
		var_dump ($redis);
		$key = $arr['login'];
		$redis -> set($key, $jsonData);
		echo "<br><br><br>";
		echo "  ".$redis -> get($key)."  ";
		ini_set('display_errors',1);
		error_reporting(E_ALL);
		echo "<br>".$jsonData."Мы в блоке try".$arr['login'];
		//}
		//catch (Exception $e) { die($e->getMessage()); }
	}
	*/


}
	
if ( isset($_POST['registration_site']) ) { include_once 'output_site.php';} //вывод записанных сайтов 
?>
</div>
  
  
</body>
</html>