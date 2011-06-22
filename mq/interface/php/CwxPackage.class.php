<?php 

/**
 * ��Key/Value����Ϣ����д���������
 * 
 * todo:ͬ��Key��δ���
 * todo:��ǩ��û�н���У�����Ƿ���ȷ
 * todo:�Է�array����string����������û�н��д�����������ֱ��ת���ַ���������ˡ�
 */

class CwxPackage
{  
	/**
	 * ���µĴ�����Ϣ
	 *
	 * @var string
	 */
	private static $error;
	/**
	 * ���µĴ������
	 *
	 * @var integer
	 */
	private static $errno;
	
	/**
	 * ��һ��Key/value����Ϣ������һ������
	 *
	 * @param string $msg
	 * @return array
	 */
    public static function unPack($msg)
    {
        $result = array();
        if( $msg === null || $msg === ''){
        	return $result;
        }
        else if( is_string($msg) == false){
        	self::$errno = -1;
        	self::$error = '����Ӧ����NULL���ַ���';
        	return false;
        }
        
        while( strlen($msg) > 0) {
        	
        	if( strlen($msg) < 3 ){
        		self::$errno = -1;
        		self::$error = '��Ϣ����󣬽��ʧ��';
        		return false;
        	}
        	$package = unpack('NkvLen/nkeyLen',$msg);
            
            if( is_array( $package ) == true){
            	
                $kvLen = $package['kvLen'];
                //����valueΪarray�����
                if( ($kvLen & 0x80000000) == true){
                    $isLoop = true;
                    $kvLen = $kvLen & 0x7fffffff;
                }
                               
                $keyLen = $package['keyLen'];
                $valueLen = $kvLen - 8 - $keyLen;
                $msg = substr($msg,6);
                
                if( strlen($msg) < $keyLen + $valueLen + 2 ){
                	self::$errno = -1;
        			self::$error = '��Ϣ����󣬽��ʧ��2';
        			return false;
                }
                
                //$package = unpack("A{$keyLen}key/atemp1/A{$valueLen}value/atemp2",$msg);
                //$key = $package['key'];
                //$value = $package['value'];
                
                //$package = unpack("A{$keyLen}key/",$msg);
                //$key = $package['key'];
                //$value = $package['value'];
                
                //��Ϊpack������Ч�ʱȽϵͣ����������о����ܿ�pack������
                if($msg[$keyLen] != "\0" || $msg[$keyLen+$valueLen+1] !="\0"){
                    self::$errno = -1;
        			self::$error = '��Ϣ����󣬽��ʧ��3';
        			return false;
                }                
                $key    =   substr($msg,0,$keyLen);
                $value  = substr($msg,$keyLen+1,$valueLen);
                
                flush();
                //exit;
                if($isLoop == true){
                	$value = self::unPack($value);
                    if($value === false){
                    	return false;
                    }
                }
                $msg = substr($msg,$kvLen-6);
                
                //����chunk��Ϣ��˵�������ͬ��key,��������������⴦��
                if($key == 'm'){
                	$result[] = $value;
                }
                else{
                	$result[$key] = $value;
                }
            }
            else{
                return false;
            }
        }
        return $result;
    }

    /**
     * ��һ����������һ��Key/Value����Ϣ��
     *
     * @param array $msg
     * @return string
     */
    public static function toPack($msg)
    {
    	$content = null;
    	
    	if(is_array($msg) == true){
    		foreach($msg as $key => $value){    			
    			//�����Ǹ�Լ������valueΪnullʱ������key.
    			//��ˣ���Ҫһ����keyʱ���뽫value���ÿ��ַ���
    			if($value === null){
    				continue;
    			}
    			//��value�������������д���
    			if(is_array($value) == true){
    				$value = self::toPack($value);
    				$keyvalue_len = strlen($key)+strlen($value)+2+6;
    				$keyvalue_len = $keyvalue_len | 0x80000000;
    			}
    			else{
    				$keyvalue_len = strlen($key)+strlen($value)+2+6;
    			}
    			$content .= pack("Nn",$keyvalue_len,strlen($key)).$key."\0".$value."\0";
    		}
    		return $content;
    	}
    	else if(is_null($msg) == true){
    		return $content;
    	}
    	else{
    		self::$errno = -1;
    		self::$error = '����Ӧ����null������';
    		return false;	
    	}
    }
    
    /**
     * �������Ĵ������
     *
     * @return integer
     */
    public static function getLastErrno()
    {
    	return self::$errno;
    }
    
    /**
     * �������Ĵ�����Ϣ
     *
     * @return string
     */
    public static function getLastError()
    {
    	return self::$error;
    }
    
    /**
     * ��ȡ��һ�������ȼ۵Ķ����ƴ�
     *
     * @param integer $num
     */
    public static function intToBuff($num)
    {
    	$ret = pack('L',$num);
    	return $ret;
    }
    
    /**
     * ��һ���ڴ���16���Ʊ�ʾ������
     *
     * @param string $buff
     * @return string
     */
    public static function buffToAscii($buff){
    	$result = null;
    	for($i=0;$i<strlen($buff);$i++){
    		$ord = ord($buff[$i]);
    		$result .= dechex($ord); 
    	}
    	return $result;
    }
    
}

?>
