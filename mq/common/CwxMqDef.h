#ifndef __CWX_MQ_DEF_H__
#define __CWX_MQ_DEF_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqDef.h
@brief MQϵ�з����ͨ�ö������ļ���
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

///�ַ����ӵ�session��Ϣ����
class CwxMqDispatchConn
{
public:
    CwxMqDispatchConn();
    ~CwxMqDispatchConn();
public:
    CwxBinLogCursor*         m_pCursor; ///<binlog�Ķ�ȡcursor
    CWX_UINT32               m_uiChunk; ///<chunk��С
    CWX_UINT64               m_ullStartSid; ///<report��sid
    CWX_UINT64               m_ullSid; ///<��ǰ���͵���sid
    bool                     m_bNext; ///<�Ƿ�����һ����Ϣ
    bool                     m_bSync; ///<�Ƿ����sync����
    CwxMqSubscribe           m_subscribe; ///<��Ϣ���Ķ���
    CWX_UINT32               m_uiWindow; ///<���ڵĴ�С
    string                   m_strSign; ///<ǩ������
    bool                     m_bZip; ///<�Ƿ�ѹ��
    set<CWX_UINT64>          m_sendingSid; ///<�����е�sid
};


///mq��fetch���ӵ�session����
class CwxMqFetchConn
{
public:
    CwxMqFetchConn();
    ~CwxMqFetchConn();
public:
    void reset();
public:
    bool            m_bWaiting; ///<�Ƿ����ڵ��ڷ�����Ϣ
    bool            m_bBlock; ///<�Ƿ�Ϊblock����
    bool            m_bCommit; ///<�Ƿ�commit���͵�queue
    CWX_UINT32      m_uiTimeout; ///<��ǰ��Ϣ��timeoutֵ
    CWX_UINT64      m_ullSendSid; ///<�Ѿ����͵�sid
    bool            m_bSent;     ///<��ǰ����Ϣ�Ƿ������
    CWX_UINT32      m_uiTaskId; ///<���ӵ�taskid
    string          m_strQueueName; ///<���е�����
};

///mq��Ϣ����
class CwxMqConfigQueue
{
public:
    CwxMqConfigQueue()
    {
        m_bCommit = false;
    }
    CwxMqConfigQueue(CwxMqConfigQueue const& item)
    {
        m_strName = item.m_strName;
        m_strUser = item.m_strUser;
        m_strPasswd = item.m_strPasswd;
        m_strSubScribe = item.m_strSubScribe;
        m_bCommit = item.m_bCommit;
    }
    CwxMqConfigQueue& operator=(CwxMqConfigQueue const& item)
    {
        if (this != &item)
        {
            m_strName = item.m_strName;
            m_strUser = item.m_strUser;
            m_strPasswd = item.m_strPasswd;
            m_strSubScribe = item.m_strSubScribe;
            m_bCommit = item.m_bCommit;
        }
        return *this;
    }
    bool operator==(CwxMqConfigQueue const& item) const
    {
        return m_strName == item.m_strName;
    };
public:
    string  m_strName; ///<���е�����
    string  m_strUser; ///<���е��û���
    string  m_strPasswd; ///<���еĿ���
    string  m_strSubScribe; ///<���е���Ϣ����
    bool    m_bCommit; ///<�Ƿ�commit����
};

///id ��Χ�ıȽ϶���
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
    ///���ص������
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
    ///��beginΪ���ݱȽϴ�С
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


///mq queue����Ϣ����
class CwxMqQueueInfo
{
public:
    CwxMqQueueInfo()
    {
        m_bCommit = false;
        m_uiDefTimeout = 0;
        m_uiMaxTimeout = 0;
        m_ullCursorSid = 0;
        m_ullLeftNum = 0;
        m_uiWaitCommitNum = 0;
        m_uiMemLogNum = 0;
        m_ucQueueState = CwxBinLogMgr::CURSOR_STATE_UNSEEK;
    }
public:
    CwxMqQueueInfo(CwxMqQueueInfo const& item)
    {
        m_strName = item.m_strName; ///<���е�����
        m_strUser = item.m_strUser; ///<���м�Ȩ���û���
        m_strPasswd = item.m_strPasswd; ///<���е��û�����
        m_bCommit = item.m_bCommit; ///<�Ƿ�commit���͵Ķ���
        m_uiDefTimeout = item.m_uiDefTimeout; ///<ȱʡ��timeoutֵ
        m_uiMaxTimeout = item.m_uiMaxTimeout; ///<����timeoutֵ
        m_strSubScribe = item.m_strSubScribe; ///<���Ĺ���
        m_ullCursorSid = item.m_ullCursorSid;
        m_ullLeftNum = item.m_ullLeftNum; ///<ʣ����Ϣ������
        m_uiWaitCommitNum = item.m_uiWaitCommitNum; ///<�ȴ�commit����Ϣ����
        m_uiMemLogNum = item.m_uiMemLogNum;
        m_ucQueueState = item.m_ucQueueState;
        m_strQueueErrMsg = item.m_strQueueErrMsg;
    }
    CwxMqQueueInfo& operator=(CwxMqQueueInfo const& item)
    {
        if (this != &item)
        {
            m_strName = item.m_strName; ///<���е�����
            m_strUser = item.m_strUser; ///<���м�Ȩ���û���
            m_strPasswd = item.m_strPasswd; ///<���е��û�����
            m_bCommit = item.m_bCommit; ///<�Ƿ�commit���͵Ķ���
            m_uiDefTimeout = item.m_uiDefTimeout; ///<ȱʡ��timeoutֵ
            m_uiMaxTimeout = item.m_uiMaxTimeout; ///<����timeoutֵ
            m_strSubScribe = item.m_strSubScribe; ///<���Ĺ���
            m_ullCursorSid = item.m_ullCursorSid;
            m_ullLeftNum = item.m_ullLeftNum; ///<ʣ����Ϣ������
            m_uiWaitCommitNum = item.m_uiWaitCommitNum; ///<�ȴ�commit����Ϣ����
            m_uiMemLogNum = item.m_uiMemLogNum;
            m_ucQueueState = item.m_ucQueueState;
            m_strQueueErrMsg = item.m_strQueueErrMsg;
        }
        return *this;
    }
public:
    string                           m_strName; ///<���е�����
    string                           m_strUser; ///<���м�Ȩ���û���
    string                           m_strPasswd; ///<���е��û�����
    bool                             m_bCommit; ///<�Ƿ�commit���͵Ķ���
    CWX_UINT32                       m_uiDefTimeout; ///<ȱʡ��timeoutֵ
    CWX_UINT32                       m_uiMaxTimeout; ///<����timeoutֵ
    string                           m_strSubScribe; ///<���Ĺ���
    CWX_UINT64                       m_ullCursorSid; ///<��ǰcursor��sid
    CWX_UINT64                       m_ullLeftNum; ///<ʣ����Ϣ������
    CWX_UINT32                       m_uiWaitCommitNum; ///<�ȴ�commit����Ϣ����
    CWX_UINT32                       m_uiMemLogNum; ///<�ڴ�����Ϣ������
    CWX_UINT8                        m_ucQueueState; ///<����״̬
    string                           m_strQueueErrMsg; //<���еĴ�����Ϣ
};

#endif
