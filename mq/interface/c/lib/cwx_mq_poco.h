#ifndef __CWX_MQ_POCO_H__
#define __CWX_MQ_POCO_H__
/*
 版权声明：
 本软件遵循GNU GPL V3（http://www.gnu.org/licenses/gpl.html），
 联系方式：email:cwinux@gmail.com；微博:http://t.sina.com.cn/cwinux
 */
/**
 @file cwx_mq_poco.h
 @brief MQ系列服务的接口协议定义。
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

///协议的消息类型定义
///RECV服务类型的消息类型定义
#define CWX_MQ_MSG_TYPE_MQ                 1 ///<数据提交消息
#define CWX_MQ_MSG_TYPE_MQ_REPLY            2 ///<数据提交消息的回复
///分发的消息类型定义
#define CWX_MQ_MSG_TYPE_SYNC_REPORT          3 ///<同步SID点报告消息类型
#define CWX_MQ_MSG_TYPE_SYNC_REPORT_REPLY    4 ///<失败返回
#define CWX_MQ_MSG_TYPE_SYNC_SESSION_REPORT  5 ///<session的报告
#define CWX_MQ_MSG_TYPE_SYNC_SESSION_REPORT_REPLY 6 ///<session报告的回复
#define CWX_MQ_MSG_TYPE_SYNC_DATA            7 ///<发送数据
#define CWX_MQ_MSG_TYPE_SYNC_DATA_REPLY       8 ///<数据的回复
#define CWX_MQ_MSG_TYPE_SYNC_DATA_CHUNK       9 ///<CHUNK模式发送数据
#define CWX_MQ_MSG_TYPE_SYNC_DATA_REPLY       10 ///<CHUNK模式数据的回复
///MQ Fetch服务类型的消息类型定义
#define CWX_MQ_MSG_TYPE_FETCH_DATA            11 ///<数据获取消息类型
#define CWX_MQ_MSG_TYPE_FETCH_DATA_REPLY      12 ///<回复数据获取消息类型
///创建mq queue消息
#define CWX_MQ_MSG_TYPE_CREATE_QUEUE          100 ///<创建MQ QUEUE的消息类型
#define CWX_MQ_MSG_TYPE_CREATE_QUEUE_REPLY     101 ///<回复创建MQ QUEUE的消息类型
///删除mq queue消息
#define CWX_MQ_MSG_TYPE_DEL_QUEUE             102 ///<删除MQ QUEUE的消息类型
#define CWX_MQ_MSG_TYPE_DEL_QUEUE_REPLY        103 ///<回复删除MQ QUEUE的消息类型
///错误消息
#define CWX_MQ_MSG_TYPE_SYNC_ERR              105  ///<数据同步错误消息

///协议的key定义
#define CWX_MQ_KEY_D "d"
#define CWX_MQ_KEY_RET  "ret"
#define CWX_MQ_KEY_SID  "sid"
#define CWX_MQ_KEY_ERR  "err"
#define CWX_MQ_KEY_B "b"
#define CWX_MQ_KEY_T  "t"
#define CWX_MQ_KEY_U  "u"
#define CWX_MQ_KEY_P "p"
#define CWX_MQ_KEY_SOURCE "source"
#define CWX_MQ_KEY_Q "q"
#define CWX_MQ_KEY_G "g"
#define CWX_MQ_KEY_CHUNK "chunk"
#define CWX_MQ_KEY_M       "m"
#define CWX_MQ_KEY_SESSION  "session"
#define CWX_MQ_KEY_NAME   "name"
#define CWX_MQ_KEY_AUTH_USER "auth_user"
#define CWX_MQ_KEY_AUTH_PASSWD "auth_passwd"
#define CWX_MQ_KEY_ZIP     "zip"

///协议错误代码定义
#define CWX_MQ_ERR_SUCCESS          0  ///<成功
#define CWX_MQ_ERR_ERROR            1   ///<错误
#define CWX_MQ_ERR_FAIL_AUTH         2 ///<鉴权失败
#define CWX_MQ_ERR_LOST_SYNC        3 ///<失去了同步状态

/**
 *@brief 形成mq的一个消息包
 *@param [in] writer package的writer。
 *@param [in] uiTaskId task-id,回复的时候会返回。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] data msg的data。
 *@param [in] type msg的type。
 *@param [in] user 接收mq的user，若为空，则表示没有用户。
 *@param [in] passwd 接收mq的passwd，若为空，则表示没有口令。
 *@param [in] zip  是否对数据压缩，1：压缩；0：不压缩。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_mq(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    struct CWX_KEY_VALUE_ITEM_S const* data,
    char const* user,
    char const* passwd,
    int zip,
    char* szErr2K
);
/**
 *@brief 解析mq的一个消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [out] data 返回msg的data。
 *@param [out] user 返回msg中的用户，0表示不存在。
 *@param [out] passwd 返回msg中的用户口令，0表示不存在。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_mq(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    struct CWX_KEY_VALUE_ITEM_S const** data,
    char const** user,
    char const** passwd,
    char* szErr2K);

/**
 *@brief pack mq的一个reply消息包
 *@param [in] writer package的writer。
 *@param [in] uiTaskId 收到消息的task-id，原样返回。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ret 错误代码。
 *@param [in] ullSid 消息成功接收时的sid。
 *@param [in] szErrMsg 消息失败时的错误消息。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
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
 *@brief 解析mq的一个reply消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [out] ret 返回msg的ret。
 *@param [out] ullSid 返回msg的sid。
 *@param [out] szErrMsg 返回msg的err-msg。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_mq_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    CWX_UINT64* ullSid,
    char const** szErrMsg,
    char* szErr2K);

/**
 *@brief pack mq的report消息包
 *@param [in] writer package的writer。
 *@param [in] uiTaskId task-id。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ullSid 同步的sid。
 *@param [in] bNewly 是否从当前binlog开始接收。
 *@param [in] uiChunk chunk的大小，若是0表示不支持chunk，单位为kbyte。
 *@param [in] source 同步的source名，若不指定则不按照source同步。
 *@param [in] user 接收的mq的user，若为空，则表示没有用户。
 *@param [in] passwd 接收的mq的passwd，若为空，则表示没有口令。
 *@param [in] sign  接收的mq的签名类型，若为空，则表示不签名。
 *@param [in] zip  接收的mq是否压缩，1压缩；0不压缩。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_sync_report(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    CWX_UINT64 ullSid,
    int bNewly,
    CWX_UINT32 uiChunk,
    char const* source,
    char const* user,
    char const* passwd,
    int zip,
    char* szErr2K);
/**
 *@brief parse mq的report消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [in] ullSid 同步的sid。
 *@param [in] bNewly 是否从当前binlog开始接收。
 *@param [in] uiChunk chunk的大小，若是0表示不支持chunk，单位为kbyte。
 *@param [in] source 同步的source。
 *@param [in] user 接收的mq的user，若为空，则表示没有用户。
 *@param [in] passwd 接收的mq的passwd，若为空，则表示没有口令。
 *@param [in] sign  接收的mq的签名类型，若为空，则表示不签名。
 *@param [in] zip  接收的mq是否压缩，1压缩；0不压缩。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_sync_report(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    CWX_UINT64* ullSid,
    int* bNewly,
    CWX_UINT32* uiChunk,
    char const** source,
    char const** user,
    char const** passwd,
    int* zip,
    char* szErr2K);

/**
 *@brief pack mq的report失败时的reply消息包
 *@param [in] writer package的writer。
 *@param [in] uiTaskId 收到report的task-id。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ullSession 同步的session。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_sync_report_reply(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    CWX_UINT64 ullSession,
    char* szErr2K);
/**
 *@brief parse mq的report失败时的reply消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [in] ullSession 同步的session。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_sync_report_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    CWX_UINT64* ullSession,
    char* szErr2K);

/**
 *@brief pack mq的sync msg的消息包
 *@param [in] writer package的writer。
 *@param [in] uiTaskId task-id。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ullSid 消息的sid。
 *@param [in] uiTimeStamp 消息接收时的时间。
 *@param [in] data 消息的data。
 *@param [in] zip  接收的mq是否压缩，1压缩；0不压缩。
 *@param [in] ullSeq 消息的序列号。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_sync_data(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    CWX_UINT64 ullSid,
    CWX_UINT32 uiTimeStamp,
    struct CWX_KEY_VALUE_ITEM_S const* data,
    int zip,
    CWX_UINT64 ullSeq,
    char* szErr2K);
/**
 *@brief parse mq的sync msg的消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in out] msg_len msg的长度。
 *@param [out] ullSid 消息的sid。
 *@param [out] uiTimeStamp 消息接收时的时间。
 *@param [out] data 消息的data。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_sync_data(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    CWX_UINT64* ullSid,
    CWX_UINT32* uiTimeStamp,
    struct CWX_KEY_VALUE_ITEM_S const** data,
    char* szErr2K);

/**
 *@brief pack mq的sync msg的消息包的回复
 *@param [in] writer package的writer。
 *@param [in] uiTaskId task-id。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ullSeq 消息的序列号。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_sync_data_reply(struct CWX_PG_WRITER * writer,
    CWX_UINT32 uiTaskId,
    char* buf,
    CWX_UINT32* buf_len,
    CWX_UINT64 ullSeq,
    char* szErr2K);
/**
 *@brief parse mq的sync msg的消息包的回复
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [out] ullSeq 消息的序列号。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_sync_data_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    CWX_UINT64* ullSeq,
    char* szErr2K);

/**
 *@brief pack mq的fetch msg的消息包
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] bBlock 在没有消息的时候是否block，1：是；0：不是。
 *@param [in] queue_name 队列的名字。
 *@param [in] user 接收的mq的user，若为空，则表示没有用户。
 *@param [in] passwd 接收的mq的passwd，若为空，则表示没有口令。
 *@param [in] timeout commit队列的超时时间，若为0表示采用默认超时时间，单位为s。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_fetch_mq(struct CWX_PG_WRITER * writer,
    char* buf,
    CWX_UINT32* buf_len,
    int bBlock,
    char const* queue_name,
    char const* user,
    char const* passwd,
    char* szErr2K);
/**
 *@brief parse  mq的fetch msg的消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [in] bBlock 在没有消息的时候是否block，1：是；0：不是。
 *@param [in] queue_name 队列的名字。
 *@param [in] user 接收的mq的user，若为空，则表示没有用户。
 *@param [in] passwd 接收的mq的passwd，若为空，则表示没有口令。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_fetch_mq(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* bBlock,
    char const** queue_name,
    char const** user,
    char const** passwd,
    char* szErr2K);

/**
 *@brief pack mq的fetch msg的reply消息包
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ret 获取mq消息的状态码。
 *@param [in] szErrMsg 状态不是CWX_MQ_ERR_SUCCESS的错误消息。
 *@param [in] data 成功时，返回消息的data。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_fetch_mq_reply(struct CWX_PG_WRITER * writer,
    char* buf,
    CWX_UINT32* buf_len,
    int ret,
    char const* szErrMsg,
    struct CWX_KEY_VALUE_ITEM_S const* data,
    char* szErr2K);
/**
 *@brief parse  mq的fetch msg的reply消息包
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [in] ret 获取mq消息的状态码。
 *@param [in] szErrMsg 状态不是CWX_MQ_ERR_SUCCESS的错误消息。
 *@param [in] data 成功时，返回消息的data。
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_fetch_mq_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    char const** szErrMsg,
    struct CWX_KEY_VALUE_ITEM_S const** data,
    char* szErr2K);

/**
 *@brief pack create queue的消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] name 队列的名字
 *@param [in] user 队列的用户名
 *@param [in] passwd 队列的用户口令
 *@param [in] auth_user mq监听的用户名
 *@param [in] auth_passwd mq监听的用户口令
 *@param [in] ullSid 队列开始的sid，若为0,则采用当前最大的sid
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_create_queue(struct CWX_PG_WRITER * writer,
    char* buf,
    CWX_UINT32* buf_len,
    char const* name,
    char const* user,
    char const* passwd,
    char const* auth_user,
    char const* auth_passwd,
    CWX_UINT64 ullSid,
    char* szErr2K);

/**
 *@brief parse create队列的 reply消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [out] name 队列的名字
 *@param [out] user 队列的用户名
 *@param [out] passwd 队列的用户口令
 *@param [out] auth_user mq监听的用户名
 *@param [out] auth_passwd mq监听的用户口令
 *@param [out] ullSid 队列开始的sid，若为0,则采用当前最大的sid
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 **/
int cwx_mq_parse_create_queue(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    char const** name,
    char const** user,
    char const** passwd,
    char const** auth_user,
    char const** auth_passwd,
    CWX_UINT64* ullSid,
    char* szErr2K);

/**
 *@brief pack create队列的reply消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ret 错误代码
 *@param [in] szErrMsg 错误消息
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_create_queue_reply(struct CWX_PG_WRITER * writer,
    char* buf,
    CWX_UINT32* buf_len,
    int ret,
    char const* szErrMsg,
    char* szErr2K);

/**
 *@brief parse create队列的reply消息
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [out] ret create队列的返回code
 *@param [out] szErrMsg create队列失败时的错误消息
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_create_queue_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    char const** szErrMsg,
    char* szErr2K);

/**
 *@brief pack delete queue的消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] name 队列的名字
 *@param [in] user 队列的用户名
 *@param [in] passwd 队列的用户口令
 *@param [in] auth_user mq监听的用户名
 *@param [in] auth_passwd mq监听的用户口令
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
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
 *@brief parse  delete队列的reply消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [out] name 队列的名字
 *@param [out] user 队列的用户名
 *@param [out] passwd 队列的用户口令
 *@param [out] auth_user mq监听的用户名
 *@param [out] auth_passwd mq监听的用户口令
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
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
 *@brief pack delete队列的reply消息
 *@param [in] writer package的writer。
 *@param [out] buf 输出形成的数据包。
 *@param [in out] buf_len 传入buf的空间大小，返回形成的数据包的大小。
 *@param [in] ret 错误代码
 *@param [in] szErrMsg 错误消息
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_pack_del_queue_reply(struct CWX_PG_WRITER * writer,
    char* buf,
    CWX_UINT32* buf_len,
    int ret,
    char const* szErrMsg,
    char* szErr2K);

/**
 *@brief parse delete队列的reply消息
 *@param [in] reader package的reader。
 *@param [in] msg 接收到的mq消息，不包括msg header。
 *@param [in] msg_len msg的长度。
 *@param [out] ret delete队列的返回code
 *@param [out] szErrMsg delete队列失败时的错误消息
 *@param [out] szErr2K 出错时的错误消息，若为空则表示不获取错误消息。
 *@return CWX_MQ_ERR_SUCCESS：成功；其他都是失败
 */
int cwx_mq_parse_del_queue_reply(struct CWX_PG_READER* reader,
    char const* msg,
    CWX_UINT32 msg_len,
    int* ret,
    char const** szErrMsg,
    char* szErr2K);

///设置数据同步包的seq号
void cwx_mq_set_seq(char* szBuf, CWX_UINT64 ullSeq);
///获取数据同步包的seq号
CWX_UINT64 cwx_mq_get_seq(char const* szBuf);

#ifdef __cplusplus
}
#endif

#endif
