<? if (isset($_COOKIE['id']) & isset($_COOKIE['hash'])) 
	{ 
		mysql_connect("localhost", "root", "") or die("MySQL сервер недоступен!".mysql_error());
		mysql_select_db("registration_site") or die("Нет соединения с БД".mysql_error());
		mysql_query("SET NAMES utf8");
		$query = mysql_query("SELECT * FROM users WHERE id_users = '".$_COOKIE['id']."' LIMIT 1");
		$userdata = mysql_fetch_array($query);
		if( ($userdata['hash'] !== $_COOKIE['hash']) or ($userdata['id_users'] !== $_COOKIE['id']) )
			{
				setcookie("id", "", time() - 3600*24*30*12);
				setcookie("hash", "", time() - 3600*24*30*12);
				?><script type="text/javascript">alert("Ошибка! Попробуйте авторизироваться еще раз.");</script>
				<script type="text/javascript">location.href="index.php"</script><?
			}
		else    
			{ 
			?><script type="text/javascript">location.href="account.php"</script><?
			}
	}
	else
	{
	if (isset ($_POST['ok']) & $_POST['login']!=null & $_POST['pass']!=null)
	{ 
		mysql_connect("localhost", "root", "") or die("MySQL сервер недоступен!".mysql_error());
		mysql_select_db("registration_site") or die("Нет соединения с БД".mysql_error());
		mysql_query("SET NAMES utf8");
		$query = mysql_query("SELECT id_users, password FROM users WHERE login='".mysql_real_escape_string($_POST['login'])."' LIMIT 1");
		$data = mysql_fetch_assoc($query);
		if (!empty($data))
			{
				if( $data['password'] === md5(md5(trim($_POST['pass']))) )
				{
				$hash = md5(rand(1,100000));
				$query = mysql_query("UPDATE users SET hash='".$hash."' WHERE id_users='".$data['id_users']."'");
				setcookie("id", $data['id_users'], time()+60*60*24*30);
				setcookie("hash", $hash, time()+60*60*24*30);
				?><script type="text/javascript">location.href="account.php"</script><? 
				}
				else 	{ ?><script type="text/javascript">alert("Ошибка! Попробуйте авторизироваться еще раз.");</script><? }
			} else { ?><script type="text/javascript">alert("Ошибка! Попробуйте авторизироваться еще раз.");</script><?}
	}
	else if (isset($_POST['ok']) & $_POST['login']==null & $_POST['pass']==null)
	{?><script type="text/javascript">alert("Поля должны быть заполнены.");</script><?}
	
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
  <div class="context headerRound"> Регистрация сайтов </div>
  <div class="index">
<?
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