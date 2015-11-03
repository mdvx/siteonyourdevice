<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Тест</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style.css">
</head>
 <body>
  <div class="context headerRound"> Регистрация сайтов!! </div>
  <div class="registrationIndex">
<?php
include_once "fun.php";
   echo " <table border=0>
<form method=post action='registration.php'>
<tr><td>Имя</td> <td><input class='form' type=text name='name'></td></tr>
<tr><td>Фамилия</td> <td><input class='form' type=text name='secondName'></td></tr>
<tr><td>E-mail</td> <td><input class='form' type=text name='email'></td></tr>
<tr><td>Логин</td> <td><input class='form' type=text name='login'></td></tr>
<tr><td>Пароль</td> <td><input class='form' type=password name='pass'></td></tr>
<tr> <td colspan=2 align=center><input class='form submit' type=submit name='registrationOk' value='Зарегистрироваться'></td></tr>
</form>  ";
echo "<tr><td colspan=2><div class='link submit'><a href='index.php'>На главную</a> </div></td></tr></table>";
 registration_user ($_POST['name'], $_POST['secondName'], $_POST['email'], $_POST['login'], $_POST['pass']);
  ?>
  
  
   </div>
</body>
</html>