#ifndef __CWX_MQ_DEF_H__
#define __CWX_MQ_DEF_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
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
#include "CwxAppAioWindow.h"
#include "CwxStl.h"
#include "CwxBinLogMgr.h"
#include "CwxMqPoco.h"
#include "CwxDTail.h"
#include "CwxSTail.h"
#include "CwxTypePoolEx.h"

class CwxMqQueue;

class CwxMqDispatchConn
{
public:
    CwxMqDispatchConn(CWX_UINT32 uiSvrId,
        CWX_UINT32 uiHosId,
        CWX_UINT32 uiConnId,
        CWX_UINT32 uiWindowSize);
    ~CwxMqDispatchConn();
public:
    bool                m_bContinue; ///<�Ƿ����continue���Ŷ���Ϣ
    bool                m_bNext; ///<�Ƿ�����һ����Ϣ
    bool                m_bSync; ///<�Ƿ����sync����
    CwxMqSubscribe      m_subscribe; ///<��Ϣ���Ķ���
    CwxAppAioWindow     m_window; ///<�ַ�����
    set<CWX_UINT64>     m_recvWindow; ///<�ȴ����ܻظ���SID����
    CwxMqDispatchConn*  m_prev; ///<ǰһ������
    CwxMqDispatchConn*  m_next; ///<��һ������
};

///�ַ����ӵĹ�����
class CwxMqDispatchConnSet
{
public:
    CwxMqDispatchConnSet(CwxBinLogMgr* pBinlogMgr);
    ~CwxMqDispatchConnSet();
public:
    CwxBinLogMgr*  m_pBinlogMgr;
    map<CWX_UINT32, CwxMqDispatchConn*>  m_clientMap; ///<�첽�ַ���client
    CwxDTail<CwxMqDispatchConn>    m_connTail; ///<�ַ����ӵ�˫������
};

///mq��fetch����
class CwxMqFetchConn
{
public:
    CwxMqFetchConn();
    ~CwxMqFetchConn();
public:
    CWX_UINT32      m_uiConnId; ///<���ӵ�id
    bool            m_bBlock; ///<�Ƿ�Ϊblock����
    bool            m_bTail; ///<�Ƿ��ڵȴ�������
    CWX_UINT32      m_uiTaskId; ///<���ӵ�taskid
    CwxMqQueue*     m_pQueue; ///<���ӵĶ���
    CwxMqFetchConn* m_prev;
    CwxMqFetchConn* m_next;
};

///�ַ����ӵĹ�����
class CwxMqFetchConnSet
{
public:
    CwxMqFetchConnSet();
    ~CwxMqFetchConnSet();
public:
    CwxTypePoolEx<CwxMqFetchConn>*     m_connPool; ///<�ڴ��
    CwxDTail<CwxMqFetchConn>           m_connWaitTail; ///<�ȴ���ȡ��Ϣ������
    map<CWX_UINT32, CwxMqFetchConn*>   m_clientMap; ///<�ȴ���ȡ��Ϣ������
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
    string  m_strName; ///<���е�����
    string  m_strUser; ///<���е��û���
    string  m_strPasswd; ///<���еĿ���
    string  m_strSubScribe; ///<���е���Ϣ����
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
#endif
