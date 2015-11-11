<?php 
if (isset($_COOKIE['id']) & isset($_COOKIE['hash'])) 
	{ 
		$connect = mysqli_connect("127.0.0.1", "root", "fastogt", "registration_site") or die("MySQL сервер недоступен!".mysql_error());
		$query = mysqli_query($connect, "SELECT * FROM users WHERE id_users = '".$_COOKIE['id']."' LIMIT 1");
		$userdata = mysqli_fetch_array($query);
			if( ($userdata['hash'] !== $_COOKIE['hash']) or ($userdata['id_users'] !== $_COOKIE['id']) )
				{
					setcookie("id", "", time() - 3600*24*30*12);
					setcookie("hash", "", time() - 3600*24*30*12);
					?><script type="text/javascript">alert("Ошибка! Попробуйте авторизироваться еще раз.");</script>
					<script type="text/javascript">location.href="index.php"</script><?php
				}
			else    
				{ 
				?><script type="text/javascript">location.href="account.php"</script><?php
				}
	}
else
	{
	if (isset ($_POST['ok']) && $_POST['login']!=null && $_POST['pass']!=null)
	{ 
		//include_once 'function/mysql_connect.php';
		$connect = mysqli_connect("127.0.0.1", "root", "fastogt", "registration_site") or die("MySQL сервер недоступен!".mysql_error());
		//mysql_query("SET NAMES utf8");
		$query = mysqli_query($connect, "SELECT id_users, password, status FROM users WHERE login='".mysqli_real_escape_string($connect, $_POST['login'])."' LIMIT 1");
		$data = mysqli_fetch_assoc($query);
	if ( $data['status']==1)
	{
		if (!empty($data))
			{
				if( $data['password'] === md5(md5(trim($_POST['pass']))) )
				{
				$hash = md5(rand(1,100000));
				$query = mysqli_query($connect,"UPDATE users SET hash='".$hash."' WHERE id_users='".$data['id_users']."'");
				setcookie("id", $data['id_users'], time()+60*60*24*30);
				setcookie("hash", $hash, time()+60*60*24*30);
				?><script type="text/javascript">location.href="account.php"</script><?php
				}
				else 	{ ?><script type="text/javascript">alert("Ошибка! Попробуйте ввести данные снова..");</script><?php }
			} 
		else { ?><script type="text/javascript">alert("Ошибка! Вы неправильно ввели логин.");</script><?php }
	}
	else { ?><script type="text/javascript">alert("Ошибка! Вы не подтвердили регистрацию. Проверьте Ваш почтовый ящик.");</script><?php }
	}
	else if (isset($_POST['ok']) && $_POST['login']==null && $_POST['pass']==null)
	{?><script type="text/javascript">alert("Поля должны быть заполнены.");</script><?php  }
	
	}
?>
<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Тест</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style/style.css">
</head>
 <body>
  <div class="context headerRound"> Регистрация сайтов </div>
  <div class="index">
<?php
echo "<table border=0>
	<form method=post action='index.php'>
	<tr><td>Логин</td><td><input class='form' type=text name='login'></td></tr>
	<tr><td>Пароль</td><td><input class='form' type=password name='pass'></td></tr>
	<tr><td colspan=2 align=center><input class='form submit' type=submit name='ok' value='Войти'></td></tr>
	</form>  ";
	echo "<tr><td colspan=2><div class='link submit'><a href='registration.php'>Регистрация</a></div></td></tr> </table>";
	
?>
 </div>
 </body>
</html>