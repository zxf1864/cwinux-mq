<?php 

/**
 * ����Cwinux��Ϣͷ
 *
 * todo:û�ж�construct��setter�����Ĳ������кϷ��Լ�飻���磬datelen���ܴ���2^31,attribute���ܴ���8λ��
 * todo:û�жԴ���ͽ�����бȽ�ȫ����ݴ���
 */

class CwxMsgHead
{
	/**
	 * ��Ϣ��ʽ�汾��
	 *
	 * @var int8
	 */
    private $version;
    /**
     * ��Ϣ����
     *
     * @var int16
     */
    private $msgType;
    /**
     * ��Ϣ��ĳ���
     *
     * @var int32
     */
    private $dataLen;
    /**
     * ��Ϣ����
     *
     * @var int8
     */
    private $attribute;
    /**
     * ����ID
     *
     * @var int32
     */
    private $taskId;
    
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
     * ���캯��
     *
     * @param int16 $msgType
     * @param int32 $dataLen
     * @param int32 $taskId
     * @param int8 $attibute
     * @param int8 $version
     */
    function __construct($msgType=0,$dataLen=0,$taskId=0,$attibute=0,$version=0)
    {
        $this->version    =   $version;
        $this->msgType    =   $msgType;
        $this->dataLen    =   $dataLen;
        $this->attribute  =   $attibute;
        $this->taskId     =   $taskId;
    }

    /**
     * ����Ϣͷ���
     *
     * @return string �����Ķ����ƴ�
     */
    public function toNet()
    {
    	if($this->msgType == 0){
    		$this->error = "��Ϣ���Ͳ���Ϊ0���";
    		$this->errno = ERR_HEADER_BAD_MSG_TYPE;
    		return false;
    	}
    	//����У���
        $checkSum = ($this->version + $this->attribute + $this->msgType + $this->taskId + $this->dataLen) & 0xffff;
        //���
        $binaryData = pack('CCnNNn',$this->version,$this->attribute,$this->msgType,$this->taskId,$this->dataLen,$checkSum);
        
        return $binaryData;
    }
    
    /**
     * �Ѷ����ƴ����һ����Ϣͷ
     * 
     * @param string $data
     * @return CwxMsgHead
     */
    public function fromNet($data)
    {
    	if(strlen($data) < 1+1+2+4+4+2 ){
    		$this->errno = ERR_HEADER_BAD_HEAD_LENGTH;
    		$this->error = "���������Ϣͷ���ȴ���";
    		return false;
    	}
        $package = unpack('Cversion/Cattribute/nmsgType/NtaskId/NdataLen/ncheckSum',$data);
        if( is_array($package) == true){
        	
        	if( (($package['msgType'] + $package['dataLen'] + $package['taskId'] + $package['attribute'] + $package['version']) & 0xffff)
        	!= $package['checkSum'] ){
        		$this->errno = ERR_HEADER_BAD_CHECK_SUM;
        		$this->error = "У���벻ƥ��";
        		return false;
        	}
        	else{
        		$this->msgType = $package['msgType'];
        		$this->dataLen = $package['dataLen'];
        		$this->taskId  = $package['taskId'];
        		$this->attribute = $package['attribute'];
        		$this->version	= $package['version'];
        		return true;
        	}
        }
        else{
        	$this->errno = ERR_HEADER_UNPACK_FAILED;
    		$this->error = "���ʧ��";
        	return false;
        }
    }

    /**
     * �Ƿ�Ϊϵͳ��Ϣ
     *
     * @return boolean
     */
    public function isSysMsg()
    {
        return ($this->attribute & 0x80)>0 ? true : false;
    }

    /**
     * ��ȡ��Ϣ�峤��
     *
     * @return integer
     */
    public function getDataLen()
    {
        return $this->dataLen;
    }

    /**
     * ������Ϣ�峤��
     *
     * @param integer $dataLen
     */
    public function setDataLen($dataLen)
    {
        $this->dataLen = $dataLen;
    }

    /**
     * ��������ID
     *
     * @param integer $taskId
     */
    public function setTaskId($taskId)
    {
        $this->taskId = $taskId;
    }

    /**
     * ��ȡ����ID
     *
     * @return integer
     */
    public function getTaskId()
    {
        return $this->taskId;
    }

    /**
     * ��ȡ��Ϣ����
     *
     * @return integer
     */
    public function getMsgType()
    {
        return $this->msgType;
    }

    /**
     * ������Ϣ����
     *
     * @param integer $msgType
     */
    public function setMsgType($msgType)
    {
        $this->msgType = $msgType;
    }

    /**
     * ��ȡ��Ϣ�汾��
     *
     * @return integer
     */
    public function getVersion()
    {
        return $this->version;
    }
    
    /**
     * ������Ϣ�汾��
     *
     * @param integer $version
     */
    public function setVersion($version)
    {
        $this->version = $version;
    }

    /**
     * ������Ϣ����
     *
     * @param integer $attr
     */
    public function setAttr($attr)
    {
        $this->attribute = $attr;
    }

    /**
     * ��ȡ��Ϣ����
     *
     * @return integer
     */
    public function getAttr()
    {
        return $this->attribute;
    }

    /**
     * �Ƿ����ĳЩ����
     *
     * @param integer $attr
     * @return boolean
     */
    public function isAttr($attr)
    {
        return ($this->attribute & $attr) == $attr ? true : false;
    }
    
    /**
     * ����ĳЩ����
     *
     * @param integer $attr
     */
    public function addAttr($attr)
    {
        $this->attribute = $this->attribute | $attr;
    }
    
    /**
     * ȡ��ĳЩ����
     *
     * @param integer $attr
     */
    public function clrAttr($attr)
    {
        $this->attribute = $this->attribute & ~$attr;
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
