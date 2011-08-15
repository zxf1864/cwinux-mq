#ifndef __CWX_DISPATCH_MACRO_H__
#define __CWX_DISPATCH_MACRO_H__
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
#define CWX_MQ_DATA "data"
#define CWX_MQ_TYPE "type"
#define CWX_MQ_RET  "ret"
#define CWX_MQ_SID  "sid"
#define CWX_MQ_ERR  "err"
#define CWX_MQ_BLOCK "block"
#define CWX_MQ_TIMESTAMP  "timestamp"
#define CWX_MQ_USER  "user"
#define CWX_MQ_PASSWD "passwd"
#define CWX_MQ_SUBSCRIBE "subscribe"
#define CWX_MQ_QUEUE "queue"
#define CWX_MQ_GROUP "group"
#define CWX_MQ_CHUNK "chunk"
#define CWX_MQ_M     "m"
#define CWX_MQ_WINDOW "window"
#define CWX_MQ_SIGN   "sign"
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
#define CWX_MQ_ERR_SUCCESS          0  ///<�ɹ�
#define CWX_MQ_ERR_NO_MSG           1   ///<û������
#define CWX_MQ_ERR_INVALID_MSG      2 ///<���յ������ݰ���Ч��Ҳ���ǲ���kv�ṹ
#define CWX_MQ_ERR_BINLOG_INVALID   3///<���յ���binlog������Ч
#define CWX_MQ_ERR_NO_KEY_DATA       4 ///<���յ���binlog��û�С�data����key
#define CWX_MQ_ERR_INVALID_DATA_KV   5 ///<data�Ŀ���Ϊkey/value������ʽ�Ƿ�
#define CWX_MQ_ERR_NO_SID            6 ///<���յ���report���ݰ��У�û�С�sid����key
#define CWX_MQ_ERR_NO_RET            7 ///<���յ������ݰ��У�û�С�ret��
#define CWX_MQ_ERR_NO_ERR            8 ///<���յ������ݰ��У�û�С�err��
#define CWX_MQ_ERR_NO_TIMESTAMP      9 ///<���յ��������У�û�С�timestamp��
#define CWX_MQ_ERR_FAIL_AUTH         10 ///<��Ȩʧ��
#define CWX_MQ_ERR_INVALID_BINLOG_TYPE 11 ///<binlog��group���󣬲���Ϊ0xFFFFFFFF
#define CWX_MQ_ERR_INVALID_MSG_TYPE   12 ///<���յ�����Ϣ������Ч
#define CWX_MQ_ERR_INVALID_SID        13  ///<�ظ���sid��Ч
#define CWX_MQ_ERR_FAIL_ADD_BINLOG    14 ///<��binglog mgr�����binlogʧ��
#define CWX_MQ_ERR_NO_QUEUE        15 ///<���в�����
#define CWX_MQ_ERR_INVALID_SUBSCRIBE 16 ///<��Ч����Ϣ��������
#define CWX_MQ_ERR_INNER_ERR        17 ///<�����ڲ�����һ��Ϊ�ڴ�
#define CWX_MQ_ERR_INVALID_MD5      18 ///<MD5У��ʧ��
#define CWX_MQ_ERR_INVALID_CRC32    19 ///<CRC32У��ʧ��
#define CWX_MQ_ERR_NO_NAME          20 ///<û��name�ֶ�
#define CWX_MQ_ERR_TIMEOUT          21 ///<commit�������͵���Ϣcommit��ʱ
#define CWX_MQ_ERR_INVALID_COMMIT   22 ///<commit������Ч
#define CWX_MQ_ERR_USER_TO0_LONG     23 ///<���е��û���̫��
#define CWX_MQ_ERR_PASSWD_TOO_LONG   24 ///<���еĿ���̫��
#define CWX_MQ_ERR_NAME_TOO_LONG   25 ///<��������̫��
#define CWX_MQ_ERR_SCRIBE_TOO_LONG   26 ///<���ж��ı��ʽ̫��
#define CWX_MQ_ERR_NAME_EMPTY        27 ///<���е�����Ϊ��
#define CWX_MQ_ERR_QUEUE_EXIST       28 ///<���д���
#define CWX_MQ_ERR_LOST_SYNC         29 ///<ʧȥ��ͬ��״̬
#define CWX_MQ_ERR_INVALID_QUEUE_NAME 30 ///<��Ч�Ķ������֣�����Ϊ[a-z,A-Z,0-9,-,_]


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
