﻿#ifndef __CWX_MQ_DEF_H__
#define __CWX_MQ_DEF_H__
/*
版权声明：
    本软件遵循GNU GPL V3（http://www.gnu.org/licenses/gpl.html），
    联系方式：email:cwinux@gmail.com；微博:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqDef.h
@brief MQ系列服务的通用对象定义文件。
@author cwinux@gmail.com
@version 0.1
@date 2010-09-15
@warning
@bug
*/

#include "CwxMqMacro.h"
#include "CwxStl.h"
#include "CwxBinLogMgr.h"
#include "CwxMqPoco.h"
#include "CwxDTail.h"
#include "CwxSTail.h"
#include "CwxTypePoolEx.h"
#include "CwxAppHandler4Channel.h"
#include "CwxHostInfo.h"

class CwxMqQueue;

///mq的fetch连接的session对象
class CwxMqFetchConn{
public:
    CwxMqFetchConn();
    ~CwxMqFetchConn();
public:
    void reset();
public:
    bool            m_bWaiting; ///<是否正在等在发送信息
    bool            m_bBlock; ///<是否为block连接
    CWX_UINT32      m_uiTaskId; ///<连接的taskid
    string          m_strQueueName; ///<队列的名字
};

///mq信息对象
class CwxMqConfigQueue{
public:
    CwxMqConfigQueue(){
        m_bCommit = false;
    }
    CwxMqConfigQueue(CwxMqConfigQueue const& item){
        m_strName = item.m_strName;
        m_strUser = item.m_strUser;
        m_strPasswd = item.m_strPasswd;
        m_strSubScribe = item.m_strSubScribe;
        m_bCommit = item.m_bCommit;
    }
    CwxMqConfigQueue& operator=(CwxMqConfigQueue const& item){
        if (this != &item){
            m_strName = item.m_strName;
            m_strUser = item.m_strUser;
            m_strPasswd = item.m_strPasswd;
            m_strSubScribe = item.m_strSubScribe;
            m_bCommit = item.m_bCommit;
        }
        return *this;
    }
    bool operator==(CwxMqConfigQueue const& item) const{
        return m_strName == item.m_strName;
    };
public:
    string  m_strName; ///<队列的名字
    string  m_strUser; ///<队列的用户名
    string  m_strPasswd; ///<队列的口令
    string  m_strSubScribe; ///<队列的消息订阅
    bool    m_bCommit; ///<是否commit类型
};

///id 范围的比较对象
class CwxMqIdRange{
public:
    CwxMqIdRange(CWX_UINT32 uiBegin, CWX_UINT32 uiEnd):m_uiBegin(uiBegin),m_uiEnd(uiEnd){
    }
    CwxMqIdRange(CwxMqIdRange const& item){
        m_uiBegin = item.m_uiBegin;
        m_uiEnd = item.m_uiEnd;
    }
    CwxMqIdRange& operator=(CwxMqIdRange const& item){
        if (this != &item){
            m_uiBegin = item.m_uiBegin;
            m_uiEnd = item.m_uiEnd;
        }
        return *this;
    }
    ///有重叠就相等
    bool operator == (CwxMqIdRange const& item) const{
         if (((m_uiBegin>=item.m_uiBegin)&&(m_uiBegin<=item.m_uiEnd)) ||
             ((m_uiEnd >= item.m_uiBegin)&&(m_uiEnd<=item.m_uiEnd)))
             return true;
         if (((item.m_uiBegin >= m_uiBegin)&& (item.m_uiBegin<=m_uiEnd)) ||
             ((item.m_uiEnd >= m_uiBegin)&&(item.m_uiEnd<=m_uiEnd)))
             return true;
         return false;
    }
    ///以begin为依据比较大小
    bool operator < (CwxMqIdRange const& item) const{
        if (*this == item) return false;
        return m_uiBegin<item.m_uiBegin;
    }

    inline CWX_UINT32 getBegin() const{
        return m_uiBegin;
    }

    inline CWX_UINT32 getEnd() const{
        return m_uiEnd;
    }
private:
    CWX_UINT32      m_uiBegin;
    CWX_UINT32      m_uiEnd;
};


///mq queue的信息对象
class CwxMqQueueInfo{
public:
    CwxMqQueueInfo(){
        m_ullCursorSid = 0;
        m_ullLeftNum = 0;
        m_uiWaitCommitNum = 0;
        m_uiMemLogNum = 0;
        m_ucQueueState = CwxBinLogCursor::CURSOR_STATE_UNSEEK;
		m_bQueueLogFileValid = true;

    }
public:
    CwxMqQueueInfo(CwxMqQueueInfo const& item){
        m_strName = item.m_strName; ///<队列的名字
        m_strUser = item.m_strUser; ///<队列鉴权的用户名
        m_strPasswd = item.m_strPasswd; ///<队列的用户口令
        m_strSubScribe = item.m_strSubScribe; ///<订阅规则
        m_ullCursorSid = item.m_ullCursorSid;
        m_ullLeftNum = item.m_ullLeftNum; ///<剩余消息的数量
        m_uiWaitCommitNum = item.m_uiWaitCommitNum; ///<等待commit的消息数量
        m_uiMemLogNum = item.m_uiMemLogNum;
        m_ucQueueState = item.m_ucQueueState;
        m_strQueueErrMsg = item.m_strQueueErrMsg;
		m_bQueueLogFileValid = item.m_bQueueLogFileValid;///队列log file是否有效
		m_strQueueLogFileErrMsg = item.m_strQueueLogFileErrMsg; ///<队列log file错误信息
    }

    CwxMqQueueInfo& operator=(CwxMqQueueInfo const& item){
        if (this != &item){
            m_strName = item.m_strName; ///<队列的名字
            m_strUser = item.m_strUser; ///<队列鉴权的用户名
            m_strPasswd = item.m_strPasswd; ///<队列的用户口令
            m_strSubScribe = item.m_strSubScribe; ///<订阅规则
            m_ullCursorSid = item.m_ullCursorSid;
            m_ullLeftNum = item.m_ullLeftNum; ///<剩余消息的数量
            m_uiWaitCommitNum = item.m_uiWaitCommitNum; ///<等待commit的消息数量
            m_uiMemLogNum = item.m_uiMemLogNum;
            m_ucQueueState = item.m_ucQueueState;
            m_strQueueErrMsg = item.m_strQueueErrMsg;
			m_bQueueLogFileValid = item.m_bQueueLogFileValid;///队列log file是否有效
			m_strQueueLogFileErrMsg = item.m_strQueueLogFileErrMsg; ///<队列log file错误信息
        }
        return *this;
    }
public:
    string                           m_strName; ///<队列的名字
    string                           m_strUser; ///<队列鉴权的用户名
    string                           m_strPasswd; ///<队列的用户口令
    string                           m_strSubScribe; ///<订阅规则
    CWX_UINT64                       m_ullCursorSid; ///<当前cursor的sid
    CWX_UINT64                       m_ullLeftNum; ///<剩余消息的数量
    CWX_UINT32                       m_uiWaitCommitNum; ///<等待commit的消息数量
    CWX_UINT32                       m_uiMemLogNum; ///<内存中消息的数量
    CWX_UINT8                        m_ucQueueState; ///<队列状态
    string                           m_strQueueErrMsg; //<队列的错误信息
	bool                             m_bQueueLogFileValid;///队列log file是否有效
	string                           m_strQueueLogFileErrMsg; ///<队列log file错误信息
};

bool mqParseHostPort(string const& strHostPort, CwxHostInfo& host);

#endif
