#ifndef __CWX_MQ_MACRO_H__
#define __CWX_MQ_MACRO_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqMacro.h
@brief MQϵ�з���ĺ궨���ļ���
@author cwinux@gmail.com
@version 1.0
@date 2010-09-23
@warning
@bug
*/
#include "CwxGlobalMacro.h"
#include "CwxType.h"
#include "CwxStl.h"
#include "CwxStlFunc.h"

CWINUX_USING_NAMESPACE


///ͨ�ŵ�key����
#define CWX_MQ_D    "d"  ///<data��key
#define CWX_MQ_RET  "ret"
#define CWX_MQ_SID  "sid"
#define CWX_MQ_ERR  "err"
#define CWX_MQ_B    "b" ///<�Ƿ�block
#define CWX_MQ_T    "t" ///<ʱ���
#define CWX_MQ_U    "u" ///<user��key
#define CWX_MQ_P    "p"  ///<passwd��key
#define CWX_MQ_SUBSCRIBE "subscribe"
#define CWX_MQ_Q   "q"   ///<���е�����
#define CWX_MQ_G    "g" ///<group��key
#define CWX_MQ_CHUNK "chunk"
#define CWX_MQ_M     "m"
#define CWX_MQ_SIGN   "sign"
#define CWX_MQ_SESSION "session"
#define CWX_MQ_CRC32  "crc32"
#define CWX_MQ_MD5    "md5"
#define CWX_MQ_NAME   "name"
#define CWX_MQ_AUTH_USER "auth_user"
#define CWX_MQ_AUTH_PASSWD "auth_passwd"
#define CWX_MQ_COMMIT  "commit"
#define CWX_MQ_TIMEOUT "timeout"
#define CWX_MQ_DEF_TIMEOUT "def_timeout"
#define CWX_MQ_MAX_TIMEOUT "max_timeout"
#define CWX_MQ_UNCOMMIT "uncommit"
#define CWX_MQ_ZIP     "zip"
#define CWX_MQ_DELAY   "delay"
///������붨��
#define UNISTOR_ERR_SUCCESS          0  ///<�ɹ�
#define UNISTOR_ERR_ERROR            1 ///<�������ֵ���ͳ����
#define UNISTOR_ERR_FAIL_AUTH        2 ///<��Ȩʧ��
#define UNISTOR_ERR_LOST_SYNC        3 ///<ʧȥ��ͬ��״̬

#define CWX_MQ_ERR_SUCCESS          0  ///<�ɹ�
#define CWX_MQ_ERR_ERROR            1  ///<����
#define CWX_MQ_ERR_FAIL_AUTH        2 ///<��Ȩʧ��
#define CWX_MQ_ERR_LOST_SYNC        3 ///<ʧȥ��ͬ��״̬

#define CWX_MQ_PROXY_NO_AUTH_GROUP    100 ///<��Ϣ��groupû�б�����
#define CWX_MQ_PROXY_FORBID_GROUP     101 ///<��Ϣ��group����ֹ
#define CWX_MQ_PROXY_NO_AUTH          102 ///<��Ϣ��group����ֹ
#define CWX_MQ_PROXY_TIMEOUT          103 ///<���ͳ�ʱ
#define CWX_MQ_PROXY_MQ_INVALID       104 ///<mq���񲻿���



#define CWX_MQ_MIN_TIMEOUT_SECOND     1 ///<��С�ĳ�ʱ����
#define CWX_MQ_MAX_TIMEOUT_SECOND     1800 ///<���ĳ�ʱ����
#define CWX_MQ_DEF_TIMEOUT_SECOND     5  ///<ȱʡ�ĳ�ʱ����

#define CWX_MQ_MAX_QUEUE_NAME_LEN        64 ///<������������
#define CWX_MQ_MAX_QUEUE_USER_LEN        64 ///<���Ķ����û�����
#define CWX_MQ_MAX_QUEUE_PASSWD_LEN      64 ///<�����û������
#define CWX_MQ_MAX_QUEUE_SCRIBE_LEN      800 ///<����ı��ʽ�ĳ���

#define CWX_MQ_MAX_MSG_SIZE           10 * 1024 * 1024 ///<������Ϣ��С
#define CWX_MQ_MAX_CHUNK_KSIZE         20 * 1024 ///<����chunk size

#define CWX_MQ_ZIP_EXTRA_BUF           128


#define CWX_MQ_MAX_BINLOG_FLUSH_COUNT  10000 ///<��������ʱ������skip sid����

#endif
