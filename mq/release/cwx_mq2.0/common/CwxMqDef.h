#ifndef __CWX_MQ_DEF_H__
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

class CwxMqQueue;

class CwxMqDispatchConn
{
public:
    CwxMqDispatchConn(CwxAppHandler4Channel* handler);
    ~CwxMqDispatchConn();
public:
    CwxAppHandler4Channel*   m_handler; ///<连接对象
    CwxBinLogCursor*         m_pCursor; ///<binlog的读取cursor
    CWX_UINT32               m_uiChunk; ///<chunk大小
    CWX_UINT64               m_ullStartSid; ///<report的sid
    CWX_UINT64               m_ullSid; ///<当前发送到的sid
    bool                     m_bNext; ///<是否发送下一个消息
    bool                     m_bSync; ///<是否接受sync数据
    CwxMqSubscribe           m_subscribe; ///<消息订阅对象
    CWX_UINT32               m_uiWindow; ///<窗口的大小
    set<CWX_UINT64>          m_sendingSid; ///<发送中的sid
};


///mq的fetch连接
class CwxMqFetchConn
{
public:
    CwxMqFetchConn();
    ~CwxMqFetchConn();
public:
    bool            m_bWaiting; ///<是否正在等在发送信息
    bool            m_bBlock; ///<是否为block连接
    CWX_UINT32      m_uiTaskId; ///<连接的taskid
    CwxMqQueue*     m_pQueue; ///<连接的队列
};


class CwxMqConfigQueue
{
public:
    CwxMqConfigQueue()
    {
    }
    CwxMqConfigQueue(CwxMqConfigQueue const& item)
    {
        m_strName = item.m_strName;
        m_strUser = item.m_strUser;
        m_strPasswd = item.m_strPasswd;
        m_strSubScribe = item.m_strSubScribe;
    }
    CwxMqConfigQueue& operator=(CwxMqConfigQueue const& item)
    {
        if (this != &item)
        {
            m_strName = item.m_strName;
            m_strUser = item.m_strUser;
            m_strPasswd = item.m_strPasswd;
            m_strSubScribe = item.m_strSubScribe;
        }
        return *this;
    }
    bool operator==(CwxMqConfigQueue const& item) const
    {
        return m_strName == item.m_strName;
    };
public:
    string  m_strName; ///<队列的名字
    string  m_strUser; ///<队列的用户名
    string  m_strPasswd; ///<队列的口令
    string  m_strSubScribe; ///<队列的消息订阅
};

class CwxMqIdRange
{
public:
    CwxMqIdRange(CWX_UINT32 uiBegin, CWX_UINT32 uiEnd):m_uiBegin(uiBegin),m_uiEnd(uiEnd)
    {
    }
    CwxMqIdRange(CwxMqIdRange const& item)
    {
        m_uiBegin = item.m_uiBegin;
        m_uiEnd = item.m_uiEnd;
    }
    CwxMqIdRange& operator=(CwxMqIdRange const& item)
    {
        if (this != &item)
        {
            m_uiBegin = item.m_uiBegin;
            m_uiEnd = item.m_uiEnd;
        }
        return *this;
    }
    ///有重叠就相等
    bool operator == (CwxMqIdRange const& item) const
    {
         if (((m_uiBegin>=item.m_uiBegin)&&(m_uiBegin<=item.m_uiEnd)) ||
             ((m_uiEnd >= item.m_uiBegin)&&(m_uiEnd<=item.m_uiEnd)))
             return true;
         if (((item.m_uiBegin >= m_uiBegin)&& (item.m_uiBegin<=m_uiEnd)) ||
             ((item.m_uiEnd >= m_uiBegin)&&(item.m_uiEnd<=m_uiEnd)))
             return true;
         return false;
    }
    ///以begin为依据比较大小
    bool operator < (CwxMqIdRange const& item) const
    {
        if (*this == item) return false;
        return m_uiBegin<item.m_uiBegin;
    }

    inline CWX_UINT32 getBegin() const
    {
        return m_uiBegin;
    }

    inline CWX_UINT32 getEnd() const
    {
        return m_uiEnd;
    }
private:
    CWX_UINT32      m_uiBegin;
    CWX_UINT32      m_uiEnd;
};
#endif
