#ifndef __CWX_MQ_QUEUE_MGR_H__
#define __CWX_MQ_QUEUE_MGR_H__
/*
��Ȩ������
    �����Ϊ�������У���ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
�����������⣺
    ��Ѷ��˾������Ѷ��˾��ֱ��ҵ���������ϵ�Ĺ�˾����ʹ�ô������ԭ��ɲο���
http://it.sohu.com/20100903/n274684530.shtml
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqMgr.h
@brief MQϵ�з����MQ�������������ļ���
@author cwinux@gmail.com
@version 0.1
@date 2010-09-15
@warning
@bug
*/

#include "CwxMqMacro.h"
#include "CwxBinLogMgr.h"
#include "CwxMutexIdLocker.h"
#include "CwxMqPoco.h"
#include "CwxMsgBlock.h"
#include "CwxMqTss.h"
#include "CwxDTail.h"
#include "CwxSTail.h"
#include "CwxMqDef.h"

class CwxMqQueue
{
public:
    CwxMqQueue(CWX_UINT32 uiId,
        string strName,
        string strUser,
        string strPasswd,
        CWX_UINT64 ullStartSid,
        CwxBinLogMgr* pBinlog);
    ~CwxMqQueue();
public:
    ///0��û����Ϣ��
    ///1����ȡһ����Ϣ��
    ///2���ﵽ�������㣬��û�з�����Ϣ��
    ///-1��ʧ�ܣ�
    int getNextBinlog(CwxMqTss* pTss,
        bool bSync,
        CwxMsgBlock*&msg,
        int& err_num,
        bool& bClose);
    ///��һ������ʧ�ܵ���Ϣ���ͻ�queue
    inline void backMsg(CwxMsgBlock*& msg)
    {
        m_memMsgTail->push_head(msg);
    }
    inline CWX_UINT32 getId() const
    {
        return m_uiId;
    }
    inline string const& getName() const
    {
        return m_strName;
    }
    inline string const& getUserName() const
    {
        return m_strUser;
    }
    inline string const& getPasswd() const
    {
        return m_strPasswd;
    }
    inline CwxMqSubscribe& getSubscribe()
    {
        return m_subscribe;
    }
    inline CWX_UINT64 getCurSid() const
    {
        return m_cursor?m_cursor->getHeader().getSid():0;
    }
    inline CWX_UINT64 getMqNum()
    {
        if (!m_cursor)
        {
            if (m_ullStartSid < m_binLog->getMaxSid())
            {
                m_cursor = m_binLog->createCurser();
                if (!m_cursor)
                {
                    return 0;
                }
                int iRet = m_binLog->seek(m_cursor, m_ullStartSid);
                if (1 != iRet)
                {
                    m_binLog->destoryCurser(m_cursor);
                    m_cursor = NULL;
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }
        return m_binLog->leftLogNum(m_cursor) + m_memMsgTail->count();
    }
private:
    CWX_UINT32                       m_uiId; ///<���е�id
    string                           m_strName; ///<���е�����
    string                           m_strUser; ///<���м�Ȩ���û���
    string                           m_strPasswd; ///<���м�Ȩ�Ŀ���
    CWX_UINT64                       m_ullStartSid; ///<���п�ʼ��sid
    CwxBinLogCursor*                 m_cursor; ///<���е��α�
    CwxSTail<CwxMsgBlock>*           m_memMsgTail; ///<���е�����
    CwxBinLogMgr*                    m_binLog; ///<binlog
    CwxMqSubscribe                   m_subscribe; ///<����
};

class CwxMqQueueMgr
{
public:
    CwxMqQueueMgr();
    ~CwxMqQueueMgr();
public:
    int init(CwxBinLogMgr* binLog,
        map<string, CWX_UINT64> const& queueSid,
        map<string, CwxMqConfigQueue> const& queueInfo);
public:
    inline CwxMqQueue* getQueue(CWX_UINT32 uiQueueId) const
    {
        map<CWX_UINT32, CwxMqQueue*>::const_iterator iter = m_idQueues.find(uiQueueId);
        return iter == m_idQueues.end()?NULL:iter->second;
    }

    inline CwxMqQueue* getQueue(string const& strQueueName) const
    {
        map<string, CwxMqQueue*>::const_iterator iter = m_nameQueues.find(strQueueName);
        return iter == m_nameQueues.end()?NULL:iter->second;

    }
    inline CWX_UINT32 getQueueNum() const
    {
        return m_nameQueues.size();
    }

private:
    map<string, CwxMqQueue*>   m_nameQueues;
    map<CWX_UINT32, CwxMqQueue*> m_idQueues;
};


#endif
