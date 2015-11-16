﻿<?php

if (isset($_POST['registrationOk']))
	{
	include 'mysql_connect.php';
		
		$err = array();
		if ( trim($_POST['pass']) !== trim($_POST['pass2'])) { $err[] = "Вы неверно ввели проверочный пароль";} 
		if(empty($_POST['name']) ) { $err[] = "Вы не ввели Ваше Имя";}
		if(empty($_POST['email']) ) { $err[] = "Вы не ввели E-mail";}
		//проверяем логин
		if(empty($_POST['login']) ) { $err[] = "Вы не ввели логин";} 
			else if(!preg_match("/^[a-zA-Z0-9_]+$/",$_POST['login']) ) { $err[] = "Логин может состоять только из букв английского алфавита, цифр, \"_\"";}
			else if(strlen($_POST['login']) < 3 or strlen($_POST['login']) > 30) {$err[] = "Логин должен быть не меньше 3-х символов и не больше 30";}
		//проверяем пароль
		if(empty($_POST['pass']) ) { $err[] = "Вы не ввели пароль<br>";}
			else if(strlen($_POST['pass']) < 5 or strlen($_POST['pass']) > 30) {$err[] = "Пароль должен быть не меньше 5-ти символов и не больше 30";}
		//проверяем почту
		if($_POST['email']!= filter_var($_POST['email'], FILTER_VALIDATE_EMAIL) ) { $err[] = "Введенный e-mail задан в неправильном формате";}
		
		//проверяем уникальность пользователя
		$query = mysqli_query($connect, "SELECT * FROM users WHERE login='".clean($_POST['login'])."'");
		$result = mysqli_num_rows($query);
		
		if ($result!=false) {$err[] = "Пользователь с таким логином уже существует";}
		
		//проверяем уникальность почтового ящика
		$query = mysqli_query($connect, "SELECT * FROM users WHERE email='".filter_var($_POST['email'], FILTER_VALIDATE_EMAIL)."'");
		$result = mysqli_num_rows($query);
		
		if ($result!=false) {$err[] = "Пользователь с таким e-mail уже существует";}
		
		//нет ошибок
		if ( count($err)==0 )
		{
			$activation=md5($_POST['email'].time()); // генерируем код активации
			$pass = md5($_POST['pass']);
			$name=clean($_POST['name']); $secondName=clean($_POST['secondName']); $email=clean($_POST['email']); $login=clean($_POST['login']); 
        // Записываем в БД хеш
		$query = mysqli_query($connect,"INSERT INTO users 
				 SET login='".$login."', password='".$pass."', name='".$name."', second_name='".$secondName."', email='".$email."', 
				 activation='".$activation."', status=0");
		//посылаем письмо
		$base_url = 'http://siteonyourdevice.com/';
		$subject='Подтверждение регистрации';//тема письма
		$message = "Здравствуйте! Вы зарегистрировались на сайте siteonyourdevice.com. Для подтверждения регистрации перейдите пожалуйста по ссылке: ".$base_url.'verification_password.php?code='.$activation;
				
		$header='From: SiteOnYourDevice <donotreply@siteonyourdevice.com>'. "\r\n" .
		'Reply-To: donotreply@siteonyourdevice.com' . "\r\n" .
		'X-Mailer: PHP/' . phpversion();
		$headers .= 'Content-type: text/plain; charset=\"KOI8-R\"';
		$parameters = '-fdonotreply@siteonyourdevice.com';
						
		// На случай если какая-то строка письма длиннее 70 символов мы используем wordwrap()
		$message = wordwrap($message, 70, "\r\n");

		// Отправляем
		mail($email, $subject, $message, $header, $parameters);
		
		?><script type="text/javascript">alert("На почту отправлено письмо. Подтвердите пожалуйста регистрацию.");</script>
		<script type="text/javascript">location.href="index.php";</script><?php		
		}
		else 
		{
			echo "<form method=post action='registration.php'>
			<input type=hidden name='name' value='".$_POST['name']."'>
			<input type=hidden name='name' value='".$_POST['secondName']."'>
			<input type=hidden name='name' value='".$_POST['email']."'>
			<input type=hidden name='name' value='".$_POST['login']."'>
			</form>
			";
			echo "<br><b>При регистрации произошли следующие ошибки:</b><br>";
			foreach($err AS $error)
			{ echo $error."<br>"; }
		}
	} 

function clean($value="")
{ $value = trim($value); $value=stripcslashes($value); $value=strip_tags($value); $value=htmlspecialchars($value); return $value; 
}
 

 
?>
