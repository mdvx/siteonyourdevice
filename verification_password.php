<!DOCTYPE html > 
<html lang="ru">
<head>
<title>Подтверждение пароля</title>
<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
<link rel="stylesheet" href="style/style.css">
</head>
 <body>
  <div class="context headerRound"> Регистрация сайтов </div>
  <div class="index">
<?php


include_once 'function/mysql_connect.php';
 
 if(!empty($_GET['code']) && isset($_GET['code']))
 {
	echo "Подтверждение регистрации.<br>";
 	$code=clean($_GET['code']);
	$query=mysqli_query($connect,"SELECT id_users, status FROM users WHERE activation='".$code."'");
	$data = mysqli_fetch_array($query);
	
	if( !empty($data) )
	{
		if( $data['status']==0 )
		{
		mysqli_query($connect,"UPDATE users SET status='1' WHERE id_users='".$data['id_users']."' and activation='".$code."'");
		echo "Ваш аккаунт активирован. Спасибо."; 
		}
		else { echo "Ваш аккаунт уже активирован, нет необходимости активировать его снова."; }
	}
	else { echo "Неверный код активации."; }
 }
 
function clean($value="")
{ $value = trim($value); $value=stripcslashes($value); $value=strip_tags($value); $value=htmlspecialchars($value); return $value; 
} 
	
?>
 </div>
 </body>
</html>