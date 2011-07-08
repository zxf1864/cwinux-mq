<?php

/**
 * MQЭ��
 * 
 */

class CwxMqPoco
{
	/**
	 * ��Ϣ��ʽ�汾��
	 *
	 * @var int8
	 */
	private $version;
	
	/**
	 * ���µĴ������
	 *
	 * @var integer
	 */
	private $errno;
	
	/**
	 * ���µĴ�����Ϣ
	 *
	 * @var string
	 */
	private $error;

	/**
	 * ���캯��
	 *
	 * @param int8 ��Ϣ���ʽ�汾��
	 */
	public function __construct($version = 0)
	{
		$this->version = $version;
	}

	/**
	 * ��ȡ��Ϣ���ʽ�汾��
	 *
	 * @return integer
	 */
	public function getVersion(){
		return $this->version;
	}
	
	/**
	 * ������Ϣ���ʽ�汾��
	 *
	 * @param integer $version
	 */
	public function setVersion($version)
	{
		$this->version = $version;
	}

	/**
	 * ���������յ��ķ�������Ӧ��Ϣ
	 * 
	 * û�д���md5��crc32��У��
	 * 
	 * @param string $msg
	 * @return array or false.
	 */	
	function parserReply($msg)
	{
		$data = CwxPackage::unPack($msg);	
		if($data === false){
			$this->errno = CwxPackage::getLastErrno();
			$this->error = CwxPackage::getLastError();
			return false;
		}
		if($data['ret'] != CWX_MQ_ERR_SUCCESS){
			$this->errno = 	$data[CWX_MQ_RET];
			$this->error = $data[CWX_MQ_ERR];
			return false;
		}
		return $data;
	}
		
	/**
	 * ���һ����Ϣ
	 *
	 * @param int8 $msgType
	 * @param int32 $taskId
	 * @param array $msg
	 * @return string
	 */
	function packMsg($msgType,$taskId,$msg)
	{
		$kvPackage = CwxPackage::toPack($msg);
		$header = new CwxMsgHead($msgType,strlen($kvPackage),$taskId,0,$this->version);
		$result = $header->toNet().$kvPackage;
		return $result;
	}

	/**
	 * ���recv_data
	 *
	 * @param int $taskId
s	 * @param string $data
	 * @param integer $group
	 * @param integer $type
	 * @param string $user
	 * @param string $passwd
	 * @param string $sign
	 * @param boolean $zip
	 * @return string
	 */
	function packRecvData($taskId,$data,$group = null,$type = null,$user=null,$passwd=null,$sign=null,$zip=null)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_DATA] = $data;
		$dataArr[CWX_MQ_GROUP] = $group;
		$dataArr[CWX_MQ_TYPE] = $type;
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		
		$kvPackage = CwxPackage::toPack($dataArr);
		
		//ǩ��
		if($sign == CWX_MQ_CRC32){
			$crc32 = crc32($kvPackage);
			$crc32 = pack('L',$crc32);
			$dataArr = array();
			$dataArr[CWX_MQ_CRC32] = $crc32;
			$kvPackage .= CwxPackage::toPack($dataArr);
		}
		else if($sign == CWX_MQ_MD5){
			$md5 = md5($kvPackage,true);
			$dataArr = array();
			$dataArr[CWX_MQ_MD5] = $md5;			
			$kvPackage .= CwxPackage::toPack($dataArr);
		}
		
		//ѹ��
		if($zip == true){
			$kvPackage = gzcompress($kvPackage);
			$header = new CwxMsgHead(MSG_TYPE_RECV_DATA,strlen($kvPackage),$taskId,2,$this->version);
		}
		else{
			$header = new CwxMsgHead(MSG_TYPE_RECV_DATA,strlen($kvPackage),$taskId,null,$this->version);
		}
		//���
		$result = $header->toNet().$kvPackage;
		return $result;
	}

	/**
	 * ��� recv_data_commit
	 *
	 * @param int $taskId
	 * @param string $user
	 * @param string $passwd
	 * @return string
	 */
	public function packRecvDataCommit($taskId,$user = null,$passwd = null)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		$result = $this->packMsg(MSG_TYPE_RECV_COMMIT,$taskId,$dataArr);
		return $result;
	}
	
	/**
	 * ���report_data
	 *
	 * @param int $taskId
	 * @param int $sid
	 * @param int $chunkSize
	 * @param int $window
	 * @param string $subscribe
	 * @param string $user
	 * @param string $passwd
	 * @param string $sign
	 * @param boolean $zip
	 * @return string
	 */
	public function packReportData($taskId,$sid = null,$chunkSize=null,$window=null,$subscribe=null,$user = null,$passwd = null,$sign = null,$zip = null)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_SID] = $sid;
		$dataArr[CWX_MQ_CHUNK] = $chunkSize;
		$dataArr[CWX_MQ_WINDOW] = $window;
		$dataArr[CWX_MQ_SUBSCRIBE] = $subscribe;
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		$dataArr[CWX_MQ_SIGN] = $sign;
		$dataArr[CWX_MQ_ZIP] = $zip;
		$result = $this->packMsg(MSG_TYPE_SYNC_REPORT,$taskId,$dataArr);		
		return $result;
		
	}
	
	/**
	 * ���sync_data_reply
	 *
	 * @param integer $taskId
	 * @param integer $sid
	 * @return string
	 */
	
	public function packReportDataReply($taskId,$sid)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_SID] = $sid;		
		$result = $this->packMsg(MSG_TYPE_SYNC_DATA_REPLY,$taskId,$dataArr);
		return $result;		
	}
	
	/**
	 * ��� fetch_mq
	 *
	 * @param string $queue_name
	 * @param boolean $block
	 * @param string $user
	 * @param string $passwd
	 * @param int $timeout
	 * @return string
	 */
	public function packFetchMq($queue_name,$block=null,$user=null,$passwd=null,$timeout=null)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_BLOCK] = $block;
		$dataArr[CWX_MQ_QUEUE] = $queue_name;
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		$dataArr[CWX_MQ_TIMEOUT] = $timeout;
		$result = $this->packMsg(MSG_TYPE_FETCH_DATA,$taskId,$dataArr);
		return $result;
	}

	/**
	 * ���fech_mq_commit
	 *
	 * @param boolean $commit
	 * @param integer $delay
	 * @return string
	 */
	public function packFetchMqCommit($commit,$delay)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_COMMIT] = $commit;
		$dataArr[CWX_MQ_DELAY] = $delay;
		$result = $this->packMsg(MSG_TYPE_FETCH_COMMIT,$taskId,$dataArr);
		return $result;
	}

	/**
	 * ���create_mq
	 *
	 * @param unknown_type $name
	 * @param unknown_type $user
	 * @param unknown_type $passwd
	 * @param unknown_type $scribe
	 * @param unknown_type $auth_user
	 * @param unknown_type $auth_passwd
	 * @param unknown_type $sid
	 * @param unknown_type $commit
	 * @param unknown_type $defaultTimeout
	 * @param unknown_type $maxTimeout
	 * @return unknown
	 */
	public function packCreateQueue($name,$user,$passwd,$scribe,$auth_user,$auth_passwd,$sid=null,$commit=null,$defaultTimeout=null,$maxTimeout = null)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_NAME] = $name;
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		$dataArr[CWX_MQ_SUBSCRIBE] = $scribe;
		$dataArr[CWX_MQ_AUTH_USER] = $auth_user;
		$dataArr[CWX_MQ_AUTH_PASSWD] = $auth_passwd;

		$dataArr[CWX_MQ_SID] = $sid;
		$dataArr[CWX_MQ_COMMIT] = $commit;
		if($commit == true){
			$dataArr[CWX_MQ_DEF_TIMEOUT] = $defaultTimeout;
			$dataArr[CWX_MQ_MAX_TIMEOUT] = $maxTimeout;
		}
		$result = $this->packMsg(MSG_TYPE_CREATE_QUEUE,$taskId,$dataArr);
		return $result;
	}

	/**
	 * ���del_mq
	 *
	 * @param unknown_type $name
	 * @param unknown_type $user
	 * @param unknown_type $passwd
	 * @param unknown_type $auth_user
	 * @param unknown_type $auth_passwd
	 * @return unknown
	 */
	public function packDelQueue($name,$user,$passwd,$auth_user,$auth_passwd)
	{
		$dataArr = array();
		$dataArr[CWX_MQ_NAME] = $name;
		$dataArr[CWX_MQ_USER] = $user;
		$dataArr[CWX_MQ_PASSWD] = $passwd;
		$dataArr[CWX_MQ_AUTH_USER] = $auth_user;
		$dataArr[CWX_MQ_AUTH_PASSWD] = $auth_passwd;

		$result = $this->packMsg(MSG_TYPE_DEL_QUEUE,$taskId,$dataArr);
		return $result;
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