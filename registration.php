<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Тест</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style/style.css">
</head>
 <body>
  <div class="context headerRound"> Регистрация сайтов </div>
  <div class="registrationIndex">

<?php
echo " <table border=0>
<form method=post action='registration.php'>
<tr><td>Имя</td> <td><input class='form' type=text name='name' value='".$_POST['name']."'></td></tr>
<tr><td>Фамилия</td> <td><input class='form' type=text name='secondName' value='".$_POST['secondName']."'></td></tr>
<tr><td>E-mail</td> <td><input class='form' type=text name='email' value='".$_POST['email']."'></td></tr>
<tr><td>Логин</td> <td><input class='form' type=text name='login' value='".$_POST['login']."'></td></tr>
<tr><td>Пароль</td> <td><input class='form' type=password name='pass'></td></tr>
<tr><td>Повторите пароль</td> <td><input class='form' type=password name='pass2'></td></tr>
<tr> <td colspan=2 align=center><input class='form submit' type=submit name='registrationOk' value='Зарегистрироваться'></td></tr>
</form>  ";
echo "<tr><td colspan=2><div class='link submit'><a href='index.php'>На главную</a> </div></td></tr></table>";

include 'function/registration_user.php';

//ini_set("display_errors",1);
//error_reporting(E_ALL);
?>
  
  
   </div>
</body>
</html>