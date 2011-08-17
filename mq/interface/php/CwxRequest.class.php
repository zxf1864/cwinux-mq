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
     * ��������
     *
     * @return boolean
     */
    public function connect()
    {
    	//Ӧ���ø����ʵķ���ȥ�ж������Ƿ��Ѿ�����
    	if($this->sock == true){
    		return true;
    	}
    	$commonProtocol = getprotobyname("tcp");
        $socket = socket_create(AF_INET,SOCK_STREAM,$commonProtocol);
        
        if(socket_connect($socket,$this->host,$this->port)==false){
        	$this->errno = ERR_REQUEST_CONNECT_FAILED;
        	$this->error = "��������ʧ��[{$this->host}:{$this->port}]";
        	return false;
        }
        $this->sock = $socket;
        return true;
    }
    
    /**
     * �Ͽ�����
     *
     */
    public function close()
    {
        if($this->sock == true){
    		socket_close($this->sock);
    		$this->sock = null;
    	}
    }
        
    /**
     * �������󣬲����ػ�õ���Ϣ��
     *
     * @param package $package
     * @return string or false 
     */
    public function request($package)
    {   
        if($this->sendMsg($package) == true){
            return $this->receiveMsg();
        }
    }
    
    /**
     * ������Ϣ��
     *
     * @param package $package
     * @return boolean
     */
    public function sendMsg($package)
    {
        if(strlen($package) == 0){
            $this->errno = ERR_REQUEST_NULL_PACKAGE;
            $this->error = '��Ϣ�岻��Ϊ�մ�';
            return false;
        }
        $socket = $this->sock;
        if($socket == null){
            $this->errno = ERR_REQUEST_NULL_SOCKET;
            $this->error = '���Ӳ����ڣ����������Ƿ�����';
            return false;
        }
        $data = socket_write($socket,$package);
        if($data === false){
            $this->errno = ERR_REQUEST_SEND_FAILED;
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
     * @param package $package
     * @return string or boolean
     */
    public function receiveMsg()
    {   
        $socket = $this->sock;
        if($socket == null){
            $this->errno = ERR_REQUEST_NULL_SOCKET;
            $this->error = '���Ӳ����ڣ����������Ƿ�����';
            return false;
        }
        
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
        			$rt = socket_read($socket,$dataLen-$n,PHP_BINARY_READ);
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
        			$this->errno = ERR_REQUEST_RECEIVE_BAD_MSG;
        			$this->error = '��ȡ��Ϣ��ʧ�� �������ݳ��ȴ���';
        			return false;	
        		}
        	}
        	else{
        		$this->errno = $header->getLastErrno();
        		$this->error = $header->getLastError();
        		return false;	
        	}
        }
        else{
        	$this->errno = ERR_REQUEST_RECEIVE_BAD_MSG_HEADR;
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
   
}


?>
