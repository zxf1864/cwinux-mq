#ifndef __CWX_MQ_POCO_H__
#define __CWX_MQ_POCO_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file cwx_mq_poco.h
@brief MQϵ�з���Ľӿ�Э�鶨�塣
@author cwinux@gmail.com
@version 1.0
@date 2010-10-06
@warning
@bug
*/

#ifdef __cplusplus
extern "C" {
#endif

#include "cwx_package_reader.h"
#include "cwx_package_writer.h"
#include "cwx_msg_header.h"
#include "cwx_md5.h"
#include "cwx_crc32.h"


///Э�����Ϣ���Ͷ���
///RECV�������͵���Ϣ���Ͷ���
#define CWX_MQ_MSG_TYPE_MQ  1 ///<�����ύ��Ϣ
#define CWX_MQ_MSG_TYPE_MQ_REPLY 2 ///<�����ύ��Ϣ�Ļظ�
#define CWX_MQ_MSG_TYPE_MQ_COMMIT 3 ///<����commit��Ϣ
#define CWX_MQ_MSG_TYPE_MQ_COMMIT_REPLY 4 ///<commit��Ϣ�Ļظ�
///�ַ�����Ϣ���Ͷ���
#define CWX_MQ_MSG_TYPE_SYNC_REPORT 5 ///<ͬ��SID�㱨����Ϣ����
#define CWX_MQ_MSG_TYPE_SYNC_REPORT_REPLY 6 ///<ʧ�ܷ���
#define CWX_MQ_MSG_TYPE_SYNC_DATA 7
#define CWX_MQ_MSG_TYPE_SYNC_DATA_REPLY 8
///MQ Fetch�������͵���Ϣ���Ͷ���
#define CWX_MQ_MSG_TYPE_FETCH_DATA 9 ///<���ݻ�ȡ��Ϣ����
#define CWX_MQ_MSG_TYPE_FETCH_DATA_REPLY 10 ///<�ظ����ݻ�ȡ��Ϣ����
#define CWX_MQ_MSG_TYPE_FETCH_COMMIT  11 ///<commit ��ȡ����Ϣ
#define CWX_MQ_MSG_TYPE_FETCH_COMMIT_REPLY 12 ///<reply commit����Ϣ
///����mq queue��Ϣ
#define CWX_MQ_MSG_TYPE_CREATE_QUEUE 100 ///<����MQ QUEUE����Ϣ����
#define CWX_MQ_MSG_TYPE_CREATE_QUEUE_REPLY 101 ///<�ظ�����MQ QUEUE����Ϣ����
///ɾ��mq queue��Ϣ
#define CWX_MQ_MSG_TYPE_DEL_QUEUE  102 ///<ɾ��MQ QUEUE����Ϣ����
#define CWX_MQ_MSG_TYPE_DEL_QUEUE_REPLY 103 ///<�ظ�ɾ��MQ QUEUE����Ϣ����

///binlog�ڲ���sync binlogleixing
#define CWX_MQ_GROUP_SYNC 0XFFFFFFFF 

///Э���key����
#define CWX_MQ_KEY_DATA "data"
#define CWX_MQ_KEY_RET  "ret"
#define CWX_MQ_KEY_SID  "sid"
#define CWX_MQ_KEY_ERR  "err"
#define CWX_MQ_KEY_BLOCK "block"
#define CWX_MQ_KEY_TIMESTAMP  "timestamp"
#define CWX_MQ_KEY_USER  "user"
#define CWX_MQ_KEY_PASSWD "passwd"
#define CWX_MQ_KEY_SUBSCRIBE "subscribe"
#define CWX_MQ_KEY_QUEUE "queue"
#define CWX_MQ_KEY_GROUP "group"
#define CWX_MQ_KEY_CHUNK "chunk"
#define CWX_MQ_KEY_WINDOW "window"
#define CWX_MQ_KEY_M       "m"
#define CWX_MQ_KEY_SIGN   "sign"
#define CWX_MQ_KEY_CRC32  "crc32"
#define CWX_MQ_KEY_MD5    "md5"
#define CWX_MQ_KEY_NAME   "name"
#define CWX_MQ_KEY_AUTH_USER "auth_user"
#define CWX_MQ_KEY_AUTH_PASSWD "auth_passwd"
#define CWX_MQ_KEY_COMMIT  "commit"
#define CWX_MQ_KEY_TIMEOUT "timeout"
#define CWX_MQ_KEY_DEF_TIMEOUT "def_timeout"
#define CWX_MQ_KEY_MAX_TIMEOUT "max_timeout"
#define CWX_MQ_KEY_COMMIT "commit"
#define CWX_MQ_KEY_UNCOMMIT "uncommit"
#define CWX_MQ_KEY_ZIP     "zip"
#define CWX_MQ_KEY_DELAY   "delay"


///Э�������붨��
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
#define CWX_MQ_ERR_INVALID_BINLOG_TYPE 11 ///<binlog��type����
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


/**
*@brief �γ�mq��һ����Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId task-id,�ظ���ʱ��᷵�ء�
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] data msg��data��
*@param [in] group msg��group��
*@param [in] type msg��type��
*@param [in] user ����mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ����mq��passwd����Ϊ�գ����ʾû�п��
*@param [in] sign  ���յ�mq��ǩ�����ͣ���Ϊ�գ����ʾ��ǩ����
*@param [in] zip  �Ƿ������ѹ����1��ѹ����0����ѹ����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_mq(struct CWX_PG_WRITER * writer,
        CWX_UINT32 uiTaskId,
        char* buf,
        CWX_UINT32* buf_len,
        struct CWX_KEY_VALUE_ITEM_S const* data,
        CWX_UINT32 group,
        char const* user,
        char const* passwd,
        char const* sign,
        int         zip,
        char* szErr2K
        );
/**
*@brief ����mq��һ����Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] data ����msg��data��
*@param [out] group msg��group��
*@param [out] user ����msg�е��û���0��ʾ�����ڡ�
*@param [out] passwd ����msg�е��û����0��ʾ�����ڡ�
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_mq(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32  msg_len,
        struct CWX_KEY_VALUE_ITEM_S const** data,
        CWX_UINT32* group,
        char const** user,
        char const** passwd,
        char* szErr2K);

/**
*@brief pack mq��һ��reply��Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId �յ���Ϣ��task-id��ԭ�����ء�
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret ������롣
*@param [in] ullSid ��Ϣ�ɹ�����ʱ��sid��
*@param [in] szErrMsg ��Ϣʧ��ʱ�Ĵ�����Ϣ��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_mq_reply(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    int ret,
    CWX_UINT64 ullSid,
    char const* szErrMsg,
    char* szErr2K);

/**
*@brief ����mq��һ��reply��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] ret ����msg��ret��
*@param [out] ullSid ����msg��sid��
*@param [out] szErrMsg ����msg��err-msg��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_mq_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    CWX_UINT64* ullSid,
    char const** szErrMsg,
    char* szErr2K);

/**
*@brief pack mq��commit��Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId ��Ϣ��task-id��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] user ���յ�mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ���յ�mq��passwd����Ϊ�գ����ʾû�п��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_commit(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    char const* user,
    char const* passwd,
    char* szErr2K);
/**
*@brief ����mq��һ��commit��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] user ����msg�е��û���0��ʾ�����ڡ�
*@param [out] passwd ����msg�е��û����0��ʾ�����ڡ�
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_commit(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    char const** user,
    char const** passwd,
    char* szErr2K);


/**
*@brief pack mq��commit reply����Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId �յ���Ϣ��task-id��ԭ�����ء�
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret ִ��״̬�롣
*@param [in] szErrMsg ִ��ʧ��ʱ�Ĵ�����Ϣ��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_commit_reply(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    int ret,
    char const* szErrMsg,
    char* szErr2K);

/**
*@brief ����mq��һ��commit reply��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] ret ִ��״̬�롣
*@param [out] szErrMsg ִ��ʧ��ʱ�Ĵ�����Ϣ��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_commit_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    char const** szErrMsg,
    char* szErr2K);

/**
*@brief pack mq��report��Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId task-id��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ullSid ͬ����sid��
*@param [in] bNewly �Ƿ�ӵ�ǰbinlog��ʼ���ա�
*@param [in] uiChunk chunk�Ĵ�С������0��ʾ��֧��chunk����λΪkbyte��
*@param [in] subscribe ���ĵ���Ϣ���͡�
*@param [in] user ���յ�mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ���յ�mq��passwd����Ϊ�գ����ʾû�п��
*@param [in] sign  ���յ�mq��ǩ�����ͣ���Ϊ�գ����ʾ��ǩ����
*@param [in] zip  ���յ�mq�Ƿ�ѹ����1ѹ����0��ѹ����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_sync_report(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    CWX_UINT64 ullSid,
    int      bNewly,
    CWX_UINT32 uiChunk,
    char const* subscribe,
    char const* user,
    char const* passwd,
    char const* sign,
    int        zip,
    char* szErr2K);
/**
*@brief parse mq��report��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [in] ullSid ͬ����sid��
*@param [in] bNewly �Ƿ�ӵ�ǰbinlog��ʼ���ա�
*@param [in] uiChunk chunk�Ĵ�С������0��ʾ��֧��chunk����λΪkbyte��
*@param [in] subscribe ���ĵ���Ϣ���͡�
*@param [in] user ���յ�mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ���յ�mq��passwd����Ϊ�գ����ʾû�п��
*@param [in] sign  ���յ�mq��ǩ�����ͣ���Ϊ�գ����ʾ��ǩ����
*@param [in] zip  ���յ�mq�Ƿ�ѹ����1ѹ����0��ѹ����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_sync_report(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    CWX_UINT64* ullSid,
    int*       bNewly,
    CWX_UINT32* uiChunk,
    char const** subscribe,
    char const** user,
    char const** passwd,
    char const** sign,
    int*        zip,
    char* szErr2K);

/**
*@brief pack mq��reportʧ��ʱ��reply��Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId �յ�report��task-id��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret reportʧ�ܵĴ�����롣
*@param [in] ullSid report��sid��
*@param [in] szErrMsg reportʧ�ܵ�ԭ��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_sync_report_reply(struct CWX_PG_WRITER * writer,
        CWX_UINT32 uiTaskId,
        char* buf,
        CWX_UINT32* buf_len,
        int ret,
        CWX_UINT64 ullSid,
        char const* szErrMsg,
        char* szErr2K);
/**
*@brief parse mq��reportʧ��ʱ��reply��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [in] ret reportʧ�ܵĴ�����롣
*@param [in] ullSid report��sid��
*@param [in] szErrMsg reportʧ�ܵ�ԭ��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_sync_report_reply(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32 msg_len,
        int* ret,
        CWX_UINT64* ullSid,
        char const** szErrMsg,
        char* szErr2K);

/**
*@brief pack mq��sync msg����Ϣ��
*@param [in] writer package��writer��
*@param [in] uiTaskId task-id��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ullSid ��Ϣ��sid��
*@param [in] uiTimeStamp ��Ϣ����ʱ��ʱ�䡣
*@param [in] data ��Ϣ��data��
*@param [in] group ��Ϣ��group��
*@param [in] sign  ���յ�mq��ǩ�����ͣ���Ϊ�գ����ʾ��ǩ����
*@param [in] zip  ���յ�mq�Ƿ�ѹ����1ѹ����0��ѹ����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_sync_data(struct CWX_PG_WRITER * writer,
        CWX_UINT32 uiTaskId,
        char* buf,
        CWX_UINT32* buf_len,
        CWX_UINT64 ullSid,
        CWX_UINT32 uiTimeStamp,
        struct CWX_KEY_VALUE_ITEM_S const* data,
        CWX_UINT32 group,
        char const* sign,
        int       zip,
        char* szErr2K);
/**
*@brief parse mq��sync msg����Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in out] msg_len msg�ĳ��ȡ�
*@param [out] ullSid ��Ϣ��sid��
*@param [out] uiTimeStamp ��Ϣ����ʱ��ʱ�䡣
*@param [out] data ��Ϣ��data��
*@param [out] group ��Ϣ��group��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_sync_data(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32 msg_len,
        CWX_UINT64* ullSid,
        CWX_UINT32* uiTimeStamp,
        struct CWX_KEY_VALUE_ITEM_S const** data,
        CWX_UINT32* group,
        char* szErr2K);

/**
*@brief pack mq��sync msg����Ϣ���Ļظ�
*@param [in] writer package��writer��
*@param [in] uiTaskId task-id��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ullSid ��Ϣ��sid��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_sync_data_reply(struct CWX_PG_WRITER * writer,
        CWX_UINT32 uiTaskId,
        char* buf,
        CWX_UINT32* buf_len,
        CWX_UINT64 ullSid,
        char* szErr2K);
/**
*@brief parse mq��sync msg����Ϣ���Ļظ�
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [in] ullSid ��Ϣ��sid��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_sync_data_reply(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32 msg_len,
        CWX_UINT64* ullSid,
        char* szErr2K);

/**
*@brief pack mq��fetch msg����Ϣ��
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] bBlock ��û����Ϣ��ʱ���Ƿ�block��1���ǣ�0�����ǡ�
*@param [in] queue_name ���е����֡�
*@param [in] user ���յ�mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ���յ�mq��passwd����Ϊ�գ����ʾû�п��
*@param [in] timeout commit���еĳ�ʱʱ�䣬��Ϊ0��ʾ����Ĭ�ϳ�ʱʱ�䣬��λΪs��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_fetch_mq(struct CWX_PG_WRITER * writer,
        char* buf,
        CWX_UINT32* buf_len,
        int bBlock,
        char const* queue_name,
        char const* user,
        char const* passwd,
        CWX_UINT32  timeout,
        char* szErr2K);
/**
*@brief parse  mq��fetch msg����Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [in] bBlock ��û����Ϣ��ʱ���Ƿ�block��1���ǣ�0�����ǡ�
*@param [in] queue_name ���е����֡�
*@param [in] user ���յ�mq��user����Ϊ�գ����ʾû���û���
*@param [in] passwd ���յ�mq��passwd����Ϊ�գ����ʾû�п��
*@param [in] timeout commit���еĳ�ʱʱ�䣬��Ϊ0��ʾ����Ĭ�ϳ�ʱʱ�䣬��λΪs��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_fetch_mq(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32 msg_len,
        int* bBlock,
        char const** queue_name,
        char const** user,
        char const** passwd,
        CWX_UINT32*  timeout,
        char* szErr2K);

/**
*@brief pack mq��fetch msg��reply��Ϣ��
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret ��ȡmq��Ϣ��״̬�롣
*@param [in] szErrMsg ״̬����CWX_MQ_ERR_SUCCESS�Ĵ�����Ϣ��
*@param [in] ullSid �ɹ�ʱ��������Ϣ��sid��
*@param [in] uiTimeStamp �ɹ�ʱ��������Ϣ��ʱ�����
*@param [in] data �ɹ�ʱ��������Ϣ��data��
*@param [in] group �ɹ�ʱ��������Ϣ��group��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_fetch_mq_reply(struct CWX_PG_WRITER * writer,
        char* buf,
        CWX_UINT32* buf_len,
        int  ret,
        char const* szErrMsg,
        CWX_UINT64 ullSid,
        CWX_UINT32 uiTimeStamp,
        struct CWX_KEY_VALUE_ITEM_S const* data,
        CWX_UINT32 group,
        char* szErr2K);
/**
*@brief parse  mq��fetch msg��reply��Ϣ��
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [in] ret ��ȡmq��Ϣ��״̬�롣
*@param [in] szErrMsg ״̬����CWX_MQ_ERR_SUCCESS�Ĵ�����Ϣ��
*@param [in] ullSid �ɹ�ʱ��������Ϣ��sid��
*@param [in] uiTimeStamp �ɹ�ʱ��������Ϣ��ʱ�����
*@param [in] data �ɹ�ʱ��������Ϣ��data��
*@param [in] group �ɹ�ʱ��������Ϣ��group��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_fetch_mq_reply(struct CWX_PG_READER* reader,
        char const* msg,
        CWX_UINT32 msg_len,
        int*  ret,
        char const** szErrMsg,
        CWX_UINT64* ullSid,
        CWX_UINT32* uiTimeStamp,
        struct CWX_KEY_VALUE_ITEM_S const** data,
        CWX_UINT32* group,
        char* szErr2K);

/**
*@brief pack commit���Ͷ��е�commit��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] commit �Ƿ�commit��1��commit��0��ȡ��commit
*@param [in] delay uncommit�ǣ�delay��������
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_fetch_mq_commit(struct CWX_PG_WRITER * writer,
        char* buf,
        CWX_UINT32* buf_len,
        int commit,
        CWX_UINT32 delay,
        char* szErr2K);

/**
*@brief parse  commit���Ͷ��е�commit��Ϣ
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] commit �Ƿ�commit��1��commit��0��ȡ��commit
*@param [out] delay uncommit�ǣ�delay��������
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_fetch_mq_commit(struct CWX_PG_READER* reader,
                                 char const* msg,
                                 CWX_UINT32 msg_len,
                                 int*  commit,
                                 CWX_UINT32* delay,
                                 char* szErr2K);

/**
*@brief pack commit���Ͷ��е�commit reply��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret �������
*@param [in] szErrMsg ������Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_fetch_mq_commit_reply(struct CWX_PG_WRITER * writer,
                                  char* buf,
                                  CWX_UINT32* buf_len,
                                  int  ret,
                                  char const* szErrMsg,
                                  char* szErr2K);
/**
*@brief parse  commit���Ͷ��е�commit reply��Ϣ
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] ret commit�ķ���code
*@param [out] szErrMsg commitʧ��ʱ�Ĵ�����Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_fetch_mq_commit_reply(struct CWX_PG_READER* reader,
                                       char const* msg,
                                       CWX_UINT32 msg_len,
                                       int*  ret,
                                       char const** szErrMsg,
                                       char* szErr2K);
/**
*@brief pack create queue����Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] name ���е�����
*@param [in] user ���е��û���
*@param [in] passwd ���е��û�����
*@param [in] scribe ���е���Ϣ���Ĺ���
*@param [in] auth_user mq�������û���
*@param [in] auth_passwd mq�������û�����
*@param [in] ullSid ���п�ʼ��sid����Ϊ0,����õ�ǰ����sid
*@param [in] commit �Ƿ�Ϊcommit���͵Ķ��У�1���ǣ�0�����ǡ�
*@param [in] uiDefTimeout ��Ϣ���е�ȱʡ��ʱʱ�䣬����0�������ϵͳĬ��ȱʡ��ʱʱ�䡣��λΪs��
*@param [in] uiMaxTimeout ��Ϣ���е����ʱʱ�䣬����0�������ϵͳĬ�����ʱʱ�䡣��λΪs��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_create_queue(struct CWX_PG_WRITER * writer,
                             char* buf,
                             CWX_UINT32* buf_len,
                             char const* name,
                             char const* user,
                             char const* passwd,
                             char const* scribe,
                             char const* auth_user,
                             char const* auth_passwd,
                             CWX_UINT64  ullSid,
                             int  commit,
                             CWX_UINT32 uiDefTimeout,
                             CWX_UINT32 uiMaxTimeout,
                             char* szErr2K);

/**
*@brief parse create���е� reply��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [out] name ���е�����
*@param [out] user ���е��û���
*@param [out] passwd ���е��û�����
*@param [out] scribe ���е���Ϣ���Ĺ���
*@param [out] auth_user mq�������û���
*@param [out] auth_passwd mq�������û�����
*@param [out] ullSid ���п�ʼ��sid����Ϊ0,����õ�ǰ����sid
*@param [out] commit �Ƿ�Ϊcommit���͵Ķ��У�1���ǣ�0�����ǡ�
*@param [out] uiDefTimeout ��Ϣ���е�ȱʡ��ʱʱ�䣬����0�������ϵͳĬ��ȱʡ��ʱʱ�䡣��λΪs��
*@param [out] uiMaxTimeout ��Ϣ���е����ʱʱ�䣬����0�������ϵͳĬ�����ʱʱ�䡣��λΪs��
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
**/
int cwx_mq_parse_create_queue(struct CWX_PG_READER* reader,
                              char const* msg,
                              CWX_UINT32 msg_len,
                              char const** name,
                              char const** user,
                              char const** passwd,
                              char const** scribe,
                              char const** auth_user,
                              char const** auth_passwd,
                              CWX_UINT64*  ullSid,
                              int*  commit,
                              CWX_UINT32* uiDefTimeout,
                              CWX_UINT32* uiMaxTimeout,
                              char* szErr2K);


/**
*@brief pack create���е�reply��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret �������
*@param [in] szErrMsg ������Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_create_queue_reply(struct CWX_PG_WRITER * writer,
                         char* buf,
                         CWX_UINT32* buf_len,
                         int  ret,
                         char const* szErrMsg,
                         char* szErr2K);

/**
*@brief parse create���е�reply��Ϣ
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] ret create���еķ���code
*@param [out] szErrMsg create����ʧ��ʱ�Ĵ�����Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_create_queue_reply(struct CWX_PG_READER* reader,
                          char const* msg,
                          CWX_UINT32 msg_len,
                          int*  ret,
                          char const** szErrMsg,
                          char* szErr2K);


/**
*@brief pack delete queue����Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] name ���е�����
*@param [in] user ���е��û���
*@param [in] passwd ���е��û�����
*@param [in] auth_user mq�������û���
*@param [in] auth_passwd mq�������û�����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_del_queue(struct CWX_PG_WRITER * writer,
                 char* buf,
                 CWX_UINT32* buf_len,
                 char const* name,
                 char const* user,
                 char const* passwd,
                 char const* auth_user,
                 char const* auth_passwd,
                 char* szErr2K);

/**
*@brief parse  delete���е�reply��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [out] name ���е�����
*@param [out] user ���е��û���
*@param [out] passwd ���е��û�����
*@param [out] auth_user mq�������û���
*@param [out] auth_passwd mq�������û�����
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
**/
int cwx_mq_parse_del_queue(struct CWX_PG_READER* reader,
                              char const* msg,
                              CWX_UINT32 msg_len,
                              char const** name,
                              char const** user,
                              char const** passwd,
                              char const** auth_user,
                              char const** auth_passwd,
                              char* szErr2K);


/**
*@brief pack delete���е�reply��Ϣ
*@param [in] writer package��writer��
*@param [out] buf ����γɵ����ݰ���
*@param [in out] buf_len ����buf�Ŀռ��С�������γɵ����ݰ��Ĵ�С��
*@param [in] ret �������
*@param [in] szErrMsg ������Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_pack_del_queue_reply(struct CWX_PG_WRITER * writer,
                         char* buf,
                         CWX_UINT32* buf_len,
                         int  ret,
                         char const* szErrMsg,
                         char* szErr2K);

/**
*@brief parse delete���е�reply��Ϣ
*@param [in] reader package��reader��
*@param [in] msg ���յ���mq��Ϣ��������msg header��
*@param [in] msg_len msg�ĳ��ȡ�
*@param [out] ret delete���еķ���code
*@param [out] szErrMsg delete����ʧ��ʱ�Ĵ�����Ϣ
*@param [out] szErr2K ����ʱ�Ĵ�����Ϣ����Ϊ�����ʾ����ȡ������Ϣ��
*@return CWX_MQ_ERR_SUCCESS���ɹ�����������ʧ��
*/
int cwx_mq_parse_del_queue_reply(struct CWX_PG_READER* reader,
                          char const* msg,
                          CWX_UINT32 msg_len,
                          int*  ret,
                          char const** szErrMsg,
                          char* szErr2K);


#ifdef __cplusplus
}
#endif


#endif
