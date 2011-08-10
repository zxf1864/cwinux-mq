<?php 

/**
 * ��װ�������
 * 
 * todo:�ж������Ƿ���ڵķ������ڼ�
 * todo:�Ƿ�Ӧ��ʹ��Listen?
 *
 */

class CwxRequest
{
	/**
	 * ������HOST
	 *
	 * @var string
	 */
    private $host;
    
    /**
     * �������˿�
     *
     * @var integer
     */
    private $port;
    
    /**
     * socket����
     *
     * @var resource 
     */
    private $sock;
    
    /**
     * ���캯��
     *
     * @param string $host
     * @param integer $port
     */
    public function __construct($host,$port)
    {
        $this->host = $host;
        $this->port = $port;
    }
    
    /**
     * ������Ϣ
     *
     * @var string
     */
    private $error;
    /**
     * �������
     *
     * @var integer
     */
    private $errno;

    /**
     * ����socket����
     *
     * @return boolean
     */
    private function connect()
    {
    	//Ӧ���ø����ʵķ���ȥ�ж������Ƿ��Ѿ�����
    	if($this->sock == true){
    		return true;
    	}
    	$commonProtocol = getprotobyname("tcp");
        $socket = socket_create(AF_INET,SOCK_STREAM,$commonProtocol);
        
        if(socket_connect($socket,$this->host,$this->port)==false){
        	$this->errno = -1;
        	$this->error = "��������ʧ��[{$this->host}:{$this->port}]";
        	return false;
        }
        $this->sock = $socket;
        return true;
    }
    
    /**
     * �������󣬲����ػ�õ���Ϣ��
     *
     * @param unknown_type $package
     * @return unknown
     */
    public function request($package)
    {   
        $ret =	$this->connect();
        if($ret == false){
        	return false;
        }
        
        $socket = $this->sock;
        $data = socket_write($socket,$package);

        $rdata = socket_read($socket,14,PHP_BINARY_READ );       
        $n = strlen($rdata);
        if($n == 14){
        	$header = new CwxMsgHead();
        	$ret = $header->fromNet($rdata);        	
        	if($ret == true){
        		
        		$dataLen = $header->getDataLen(); 
        		
        		$rdata = null;
        		$n = 0;
        		
        		while( $n < $dataLen){
        			$rt = socket_read($socket,512,PHP_BINARY_READ);
        			$rdata .= $rt;
        			$n = $n+strlen($rt); 
        		}
        		
        		if($n == $dataLen){
        			//����ѹ������Ϣ�����н�ѹ������
        			if( ($header->getAttr() & 2) == true){
        				$rdata = gzuncompress($rdata);
        			}
        			return $rdata;
        		}
        		else{
        			$this->errno = -1;
        			$this->error = '��ȡ��Ϣ��ʧ�� �������ݳ��ȴ���';
        			return false;	
        		}
        	}
        	else{
        		$this->errno = -1;
        		$this->error = $header->getLastError();
        		return false;	
        	}
        }
        else{
        	$this->errno = -1;
        	$this->error = '��ȡ��Ϣͷʧ�� �������ݳ��ȴ���';
        	return false;
        }
    }
    
    
    public function getSocket()
    {
        $commonProtocol = getprotobyname("tcp");
        $socket = socket_create(AF_INET,SOCK_STREAM,$commonProtocol);
        
        if(socket_connect($socket,$this->host,$this->port)==false){
        	$this->errno = -1;
        	$this->error = "��������ʧ��[{$this->host}:{$this->port}]";
        	return false;
        }
        return $socket;
    }
    /**
     * ������Ϣ��
     *
     * @param unknown_type $package
     * @return unknown
     */
    public function sendMsg($socket,$package)
    {
        if(strlen($package) == 0){
            $this->errno = -1;
            $this->error = '��Ϣ�岻��Ϊ�մ�';
            return false;
        }
        
        $data = socket_write($socket,$package);
        if($data === false){
            $this->errno = -1;
            $this->error = '������Ϣʧ�ܣ����������Ƿ�����';
            return false;
        }
        else{
            return true;
        }
    }
    
    /**
     * ������Ϣ��
     *
     * @param unknown_type $package
     * @return unknown
     */
    public function receiveMsg($socket)
    {   
        $rdata = socket_read($socket,14,PHP_BINARY_READ );       
        $n = strlen($rdata);
        if($n == 14){
        	$header = new CwxMsgHead();
        	$ret = $header->fromNet($rdata);        	
        	if($ret == true){
        		
        		$dataLen = $header->getDataLen(); 
        		
        		$rdata = null;
        		$n = 0;
        		
        		while( $n < $dataLen){
        			$rt = socket_read($socket,512,PHP_BINARY_READ);
        			$rdata .= $rt;
        			$n = $n+strlen($rt); 
        		}
        		
        		if($n == $dataLen){
        			//����ѹ������Ϣ�����н�ѹ������
        			if( ($header->getAttr() & 2) == true){
        				$rdata = gzuncompress($rdata);
        			}
        			return $rdata;
        		}
        		else{
        			$this->errno = -1;
        			$this->error = '��ȡ��Ϣ��ʧ�� �������ݳ��ȴ���';
        			return false;	
        		}
        	}
        	else{
        		$this->errno = -1;
        		$this->error = $header->getLastError();
        		return false;	
        	}
        }
        else{
        	$this->errno = -1;
        	$this->error = '��ȡ��Ϣͷʧ�� �������ݳ��ȴ���';
        	return false;
        }
    }
      /**
     * ��ȡ���Ĵ�����Ϣ
     *
     * @return string
     */
    public function getLastError(){
    	return $this->error;
    }
    
    /**
     * ��ȡ���Ĵ������
     *
     * @return integer
     */
    public function getLastErrno(){
    	return $this->errno;
    }
    
    /*
    public function request2($package)
    {   
        $ret =	$this->connect();
        
        if($ret == false){
        	return false;
        }
        
        $socket = $this->sock;
        $data = socket_write($socket,$package);

        $n = socket_recv($socket,$rdata,512,PHP_BINARY_READ );
        
        var_dump($n);
        if ( $n <= 0 )	{
            $rt['errcode']=-3;
            return $rt;
        }
        //socket_close($socket);        
        return $rdata;
    }*/

}


?>
