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
<title>Личный кабинет</title>
<link rel="icon" href="http://siteonyourdevice.com/favicon.png" type="image/png"/>
<link rel="shortcut icon" href="http://siteonyourdevice.com/favicon.ico" type="image/x-icon"/>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style/style.css">

</head>
 <body>
<div class="context header borderleft borderright">
 <div class="head"> <div class="left">SITE ON YOUR DEVICE  </div></div>
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
include_once 'function/mysql_connect.php'; 

$query = mysqli_query ($connect, "SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
$arr = mysqli_fetch_assoc($query);


if ( isset($_POST['private_data']) ) { include_once 'function/personal_data.php'; } //данные о пользователе

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

if (isset($_POST['deleteacc'])) { 	include_once 'function/delete_account.php'; }
if (isset($_POST['okregistr'])) { 	include_once 'function/registration_site.php'; } //регистрация сайта
if ( isset($_POST['registration_site']) ) { 	include_once 'function/output_site.php';} //вывод записанных сайтов 
if (isset($_POST['redact_opisanie'])) { 	include_once 'function/redact_opisanie.php'; } //редактирование описания сайта
if (isset($_POST['delete_site'])) { 	include_once 'function/delete_site.php'; } //удаление сайта
?>
</div>
  
  
</body>
</html>