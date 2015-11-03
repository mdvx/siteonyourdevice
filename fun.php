<?
function connect ()
  {
$host="localhost";
$user="root";
$password="fastogt";
$db="registration_site";
mysql_connect($host, $user, $password) or die("MySQL сервер недоступен!".mysql_error());
mysql_select_db($db) or die("Нет соединения с БД".mysql_error());
mysql_query("SET NAMES utf8");
  }
  
  function clean($value="")
{ $value = trim($value); $value=stripcslashes($value); $value=strip_tags($value); $value=htmlspecialchars($value); return $value; 
}
 
 
  function registration_user ($name, $secondName, $email, $login, $pass)
  {
	connect();
	
	if (isset($_POST['registrationOk']))
	{
		$err = array();
		
		if( empty($name) ) { $err[] = "Вы не ввели Ваше Имя";}
		if( empty($email) ) { $err[] = "Вы не ввели E-mail";}
		//проверяем логин
		if( empty($login) ) { $err[] = "Вы не ввели логин";} 
			else if(!preg_match("/^[a-zA-Z0-9_]+$/",$login) ) { $err[] = "Логин может состоять только из букв английского алфавита, цифр, \"_\"";}
			else if(strlen($login) < 3 or strlen($login) > 30) {$err[] = "Логин должен быть не меньше 3-х символов и не больше 30";}
		//проверяем пароль
		if( empty($pass) ) { $err[] = "Вы не ввели пароль<br>";}
			else if(strlen($pass) < 5 or strlen($pass) > 30) {$err[] = "Пароль должен быть не меньше 5-ти символов и не больше 30";}
		//проверяем почту
		if( $email!= filter_var($email, FILTER_VALIDATE_EMAIL) ) { $err[] = "Ваш e-mail задан в неправильном формате";}
		
		//проверяем пользователя
		$query = mysql_query("SELECT * FROM users WHERE login='".clean($login)."'");
		if (mysql_num_rows($query)>0) {$err[] = "Пользователь с таким логином уже существует";}
		
		//нет ошибок
		if ( count($err)==0 )
		{
			$pass = md5(md5(trim($pass)));
			$name=clean($name); $secondName=clean($secondName); $email=clean($email); $login=clean($login); 
        // Записываем в БД хеш
		$query = mysql_query ("INSERT INTO users SET login='".$login."', password='".$pass."', name='".$name."', second_name='".$secondName."',
		email='".$email."'");
		?><script type="text/javascript">location.href="index.php"</script>
		  <script type="text/javascript">alert("Авторизируйтесь пожалуйста");</script><?		
		}
		else 
		{
			echo "<br><b>При регистрации произошли следующие ошибки:</b><br>";
			foreach($err AS $error)
			{ echo $error."<br>"; }
		}
	}
	
  }
  
function registration_site($sait, $opisanieSait, $hidden)
{
	connect();
	$sait=clean($sait); $opisanieSait=clean($opisanieSait); 
		if ($sait!=null)
		{ 
			$s=mysql_query("INSERT INTO users_site SET id_users='".$hidden."', adds_site='".$sait."', description='".$opisanieSait."' ");
			echo "Сайт успешно зарегистрирован.";
			echo " <br>".mysql_error();
		}
		else 
		{ echo "Пустых полей быть не должно.";}
}
  
  function output_data()
  {
	connect();
	$W=mysql_query("SELECT * FROM users_site where id_users='".$_COOKIE['id']."'");
	$num_results = mysql_num_rows($W);
			if ($num_results!=0)
			{
			echo "<table border=1 width=100%>
				<tr><td width=30%>Адрес сайта</td>
			    <td width=70%>Описание</td></tr>";
			for ($n=0; $n<$num_results; $n++)
				{
				$arr = mysql_fetch_array($W);
				echo "<tr><td>".$arr["adds_site"]."</td>
			    <td>".$arr["description"]."</td></tr>";
				} echo "</table>";
				
			}	
  }
 
 
 function personal_data()
 {
	$query = mysql_query("SELECT * FROM users where id_users='".$_COOKIE['id']."' LIMIT 1");
	$arr = mysql_fetch_array ($query);
			echo "<table border=1 width=100%>
				<tr><td width=30%>Имя</td><td>".$arr["name"]."</td></tr>
				<tr><td width=30%>Фамилия</td><td>".$arr["second_name"]."</td></tr>
				<tr><td width=30%>E-mail</td><td>".$arr["email"]."</td></tr>
				<tr><td width=30%>Логин</td><td>".$arr["login"]."</td></tr>
				</table>";
 }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
?>
  
