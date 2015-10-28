<?  if (!isset($_COOKIE['id']) & !isset($_COOKIE['hash'])) 
	{ 		
		?><script type="text/javascript">alert("Авторизируйтесь пожалуйста");</script>
		<script type="text/javascript">location.href="index.php"</script><?
	}
	if ( isset($_POST['exit']) ) 
	{ 		
		setcookie("id", "", time() - 3600*24*30*12);
		setcookie("hash", "", time() - 3600*24*30*12);
		?><script type="text/javascript">location.href="index.php"</script><?
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
 <?
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
include_once "fun.php"; 
connect();
$query = mysql_query ("SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
$arr = mysql_fetch_assoc($query);


if ( isset($_POST['private_data']) )
{
	personal_data();
}

if ( isset($_POST['registration_data']) | isset($_POST['ok']) )
{
echo " 
<form method=post action='account.php'>
<table border=0>
<tr><td> Адрес сайта </td><td><input class='form' type=text placeholder='Адрес сайта' name='sait'></td></tr>
<tr><td> Описание сайта </td><td><textarea class='form' rows=5  placeholder='Описание сайта' name='opisanieSait'></textarea></td></tr>
<tr><td> </td> <td><input type=hidden name='hidden' value='".$arr['id_users']."'></td></tr>
<tr><td><input class='form submit' type=submit name='okregistr' value='Зарегистрировать'></td></tr>
</table>
</form>";
}

if (isset($_POST['okregistr']))  //регистрация сайта
{ 	registration_site($_POST['sait'], $_POST['opisanieSait'], $_POST['hidden']); 

if ($_POST['sait']!=null) 
	{
		//запись в базу редис
	try
		{		
		$arr = array ('name' => $arr['name'], 'secondName' => $arr['second_name'], 'login' => $arr['login'], 'password'=>  $arr['password'], 'sait' => $_POST['sait'], 'hosts' => array ('127.0.0.1', '127.0.0.2', '127.0.0.3')   );
		$jsonData = json_encode($arr);
		
		include_once "redis_connect.php";
		$key = $arr['login'];
		$redis -> set($key, $jsonData);
		print_r ($jsonData);
		}
		catch (Exception $e) { die($e->getMessage()); }
	}

}
	
if ( isset($_POST['registration_site']) ) { output_data();} //вывод записанных сайтов 
?>
</div>
  
  
</body>
</html>