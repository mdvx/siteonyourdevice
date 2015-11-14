<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Регистрация сайтов</title>
<link rel="icon" href="http://siteonyourdevice.com/favicon.png" type="image/png"/>
<link rel="shortcut icon" href="http://siteonyourdevice.com/favicon.ico" type="image/x-icon"/>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style/style.css">
</head>
 <body>
  <div class="context headerRound"> SITE ON YOUR DEVICE </div>
  <div class="registrationIndex">

<?php
echo " <table border=0 class='table'>
<form method=post action='registration.php'>
<tr><td>Имя <font color='red'> * </font></td> <td><input class='form' type=text name='name' value='".$_POST['name']."'></td></tr>
<tr><td>Фамилия <font color='red'> * </font></td> <td><input class='form' type=text name='secondName' value='".$_POST['secondName']."'></td></tr>
<tr><td>E-mail <font color='red'> * </font></td> <td><input class='form' type=text name='email' value='".$_POST['email']."'></td></tr>
<tr><td>Логин <font color='red'> * </font></td> <td><input class='form' type=text name='login' value='".$_POST['login']."'></td></tr>
<tr><td>Пароль <font color='red'> * </font></td> <td><input class='form' type=password name='pass'></td></tr>
<tr><td>Повторите пароль <font color='red'> * </font></td> <td><input class='form' type=password name='pass2'></td></tr>
<tr> <td></td> <td><input class='form submit' type=submit name='registrationOk' value='Зарегистрироваться'></td></tr> </form>  ";

echo "<tr><td></td> <td><div class='link submit'><a href='index.php'>На главную</a> </div></td></tr>
	 <tr><td colspan=2><div> <br> <font color='red'> * </font> - поля обязательные для заполнения</div></td></tr></table>";

include 'function/registration_user.php';

//ini_set("display_errors",1);
//error_reporting(E_ALL);
?>
  
  
   </div>
</body>
</html>