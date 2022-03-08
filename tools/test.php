<?php

// localhost:80/test.php?id=1

$id = '';
if (isset($_GET['id']))
{
	$id = $_GET['id'];
	$num = intval($id);
	
	if ($num % 22 === 0)   // 此次会造成超时
		sleep(5);
}
echo "Hello world, id =  $id ";

