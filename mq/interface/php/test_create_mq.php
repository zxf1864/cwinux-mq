<?php
        
		function test_create_mq(){}
        include_once('CwxMqDef.class.php');
		function __autoload($name){
			include_once($name.".class.php");
		}
		
        $host	=	'127.0.0.1';
		$port	=	9906;
		
		$queue	=	'aa';
		$user	=	'mq_admin';
		$passwd =	'mq_admin_passwd';
		$subscribe = '3:5';
		$auth_user		=	'mq_admin';
		$auth_passwd 	=	'mq_admin_passwd';
		
        $poco = new CwxMqPoco();
        $request = new CwxRequest($host,$port);
        
        $pack = $poco->packCreateQueue($queue,$user,$passwd,$subscribe,$auth_user,$auth_passwd);        
        
        $socket = $request->getSocket();
        $ret = $request->sendMsg($socket,$pack);
        $ret = $request->receiveMsg($socket);
        if($ret === false){
       		echo $request->getLastError();
       		exit;
       	}
        $r = $poco->parserReply($ret);
        if($r === false){
        	echo $poco->getLastError();
        	exit;
		}
		
		echo "<pre>";
		print_r($r);
        
?>
