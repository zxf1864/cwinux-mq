#ifndef __CWX_MQ_QUEUE_MGR_H__
#define __CWX_MQ_QUEUE_MGR_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
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
#include "CwxMqDef.h"
#include "CwxMinHeap.h"
#include "CwxMqQueueLogFile.h"

class CwxMqQueueHeapItem
{
public:
    CwxMqQueueHeapItem()
    {
        m_msg = NULL;
        m_index = -1;
        m_ttTimestamp = 0;
        m_ullSid = 0;
        m_bSend = false;
    }
    ~CwxMqQueueHeapItem()
    {
        if (m_msg) CwxMsgBlockAlloc::free(m_msg);
    }
public:
    bool operator <(CwxMqQueueHeapItem const& item) const
    {
        return m_ttTimestamp < item.m_ttTimestamp;
    }

    CWX_INT32 index() const
    {
        return m_index;
    }
    void index(CWX_INT32 index)
    {
        m_index = index;
    }
    CwxMsgBlock* msg()
    {
        return m_msg;
    }
    void msg(CwxMsgBlock* msg)
    {
        m_msg = msg;
    }
    CWX_UINT32 timestamp() const
    {
        return m_ttTimestamp;
    }
    void timestamp(CWX_UINT32 uiTimestamp)
    {
        m_ttTimestamp = uiTimestamp;
    }
    CWX_UINT64 sid() const
    {
        return m_ullSid;
    }
    void sid(CWX_UINT64 ullSid)
    {
        m_ullSid = ullSid;
    }
    bool send() const
    {
        return m_bSend;
    }
    void send(bool bSend)
    {
        m_bSend = bSend;
    }

private:
    CWX_INT32   m_index;///<heap��index
    CWX_UINT32  m_ttTimestamp; ///<��Ϣ��ʱ���
    CWX_UINT64  m_ullSid; ///<��Ϣ��sid
    bool        m_bSend; ///<��Ϣ�Ƿ������
    CwxMsgBlock*  m_msg; ///<��Ϣ
};

class CwxMqQueue
{
public:
    CwxMqQueue(string strName,
        string strUser,
        string strPasswd,
        bool    bCommit,
        string strSubscribe,
        CWX_UINT32 uiDefTimeout,
        CWX_UINT32 uiMaxTimeout,
        CwxBinLogMgr* pBinlog);
    ~CwxMqQueue();
public:
    ///0:�ɹ�;-1��ʧ��
    int init(CWX_UINT64 ullLastCommitSid,
        set<CWX_UINT64> const& uncommitSid,
        set<CWX_UINT64> const& commitSid,
        string& strErrMsg);
    ///0��û����Ϣ��
    ///1����ȡһ����Ϣ��
    ///2���ﵽ�������㣬��û�з�����Ϣ��
    ///-1��ʧ�ܣ�
    int getNextBinlog(CwxMqTss* pTss,
        CwxMsgBlock*&msg,
        CWX_UINT32 uiTimeout,
        int& err_num,
        char* szErr2K);

    ///����commit���͵Ķ��У��ύcommit��Ϣ��
    ///����ֵ��0�������ڣ�1���ɹ�.
    int commitBinlog(CWX_UINT64 ullSid, bool bCommit=true);
    ///��Ϣ������ϣ�bSend=true��ʾ�Ѿ����ͳɹ���false��ʾ����ʧ��
    ///����ֵ��0�������ڣ�1���ɹ�.
    int endSendMsg(CWX_UINT64 ullSid, bool bSend=true);
    ///���commit���Ͷ��г�ʱ����Ϣ
    void checkTimeout(CWX_UINT32 ttTimestamp);

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
    inline string const& getSubscribeRule() const
    {
        return m_strSubScribe;
    }
    inline CWX_UINT32 getDefTimeout() const
    {
        return m_uiDefTimeout;
    }
    inline CWX_UINT32 getMaxTimeout() const
    {
        return m_uiMaxTimeout;
    }
    inline bool isCommit() const
    {
        return m_bCommit;
    }
    inline CWX_UINT64 getCurSid() const
    {
        return m_cursor?m_cursor->getHeader().getSid():0;
    }
    inline CWX_UINT32 getWaitCommitNum() const
    {
        return m_uncommitMap.size();
    }
    inline map<CWX_UINT64, void*>& getUncommitMap()
    {
        return m_uncommitMap; ///<commit������δcommit����Ϣsid����
    }
    inline map<CWX_UINT64, CwxMsgBlock*>& getMemMsgMap()
    {
        return m_memMsgMap;///<����ʧ����Ϣ����
    }

    inline CwxBinLogCursor* getCursor() 
    {
        return m_cursor;
    }
    inline CWX_UINT32 getMemSidNum() const
    {
        return m_memMsgMap.size();
    }
    inline CWX_UINT32 getUncommitSidNum() const
    {
        return m_uncommitMap.size();
    }
    inline CWX_UINT64 getCursorSid() const
    {
        if (m_cursor && (CwxBinLogMgr::CURSOR_STATE_READY == m_cursor->getSeekState()))
            return m_cursor->getHeader().getSid();
        return getStartSid();
    }
    ///��ȡcursor����ʼsid
    inline CWX_UINT64 getStartSid() const
    {
        if (m_lastUncommitSid.size())
        {
           return *m_lastUncommitSid.begin() - 1;
        }
        return m_ullLastCommitSid;
    }
    ///��ȡdump��Ϣ
    void getQueueDumpInfo(CWX_UINT64& ullLastCommitSid,
        set<CWX_UINT64>& uncommitSid,
        set<CWX_UINT64>& commitSid);
    CWX_UINT64 getMqNum();
private:
    ///0��û����Ϣ��
    ///1����ȡһ����Ϣ��
    ///2���ﵽ�������㣬��û�з�����Ϣ��
    ///-1��ʧ�ܣ�
    int fetchNextBinlog(CwxMqTss* pTss,
        CwxMsgBlock*&msg,
        int& err_num,
        char* szErr2K);
private:
    string                           m_strName; ///<���е�����
    string                           m_strUser; ///<���м�Ȩ���û���
    string                           m_strPasswd; ///<���м�Ȩ�Ŀ���
    bool                             m_bCommit; ///<�Ƿ�commit���͵Ķ���
    CWX_UINT32                       m_uiDefTimeout; ///<ȱʡ��timeoutֵ
    CWX_UINT32                       m_uiMaxTimeout; ///<����timeoutֵ
    string                           m_strSubScribe; ///<���Ĺ���
    CwxBinLogMgr*                    m_binLog; ///<binlog
    CwxMinHeap<CwxMqQueueHeapItem>*  m_pUncommitMsg; ///<commit������δcommit����Ϣ
    map<CWX_UINT64, void*>           m_uncommitMap; ///<commit������δcommit����Ϣsid����
    map<CWX_UINT64, CwxMsgBlock*>    m_memMsgMap;///<����ʧ����Ϣ����
    CwxBinLogCursor*                 m_cursor; ///<���е��α�
    CwxMqSubscribe                   m_subscribe; ///<����
    CWX_UINT64                       m_ullLastCommitSid; ///<��־�ļ���¼��cursor��sid
    set<CWX_UINT64>                  m_lastUncommitSid; ///<m_ullLastCommitSid֮ǰδcommit��binlog
    set<CWX_UINT64>                  m_lastCommitSid; ///<m_ullLastCommitSid֮��commit��binlog
};


class CwxMqQueueMgr
{
public:
    enum
    {
        MQ_SWITCH_LOG_NUM = 100000,
        MQ_MAX_SWITCH_LOG_INTERNAL = 1800
    };
public:
    CwxMqQueueMgr(string const& strQueueLogFile,
        CWX_UINT32 uiMaxFsyncNum);
    ~CwxMqQueueMgr();
public:
    //0:�ɹ���-1��ʧ��
    int init(CwxBinLogMgr* binLog);
public:
    ///0��û����Ϣ��
    ///1����ȡһ����Ϣ��
    ///2���ﵽ�������㣬��û�з�����Ϣ��
    ///-1��ʧ�ܣ�
    ///-2�����в�����
    int getNextBinlog(CwxMqTss* pTss, ///<tss����
        string const& strQueue, ///<���е�����
        CwxMsgBlock*&msg, ///<��Ϣ
        CWX_UINT32 uiTimeout, ///<��Ϣ�ĳ�ʱʱ��
        int& err_num, ///<������Ϣ
        bool& bCommitType, ///<�Ƿ�Ϊcommit���͵Ķ���
        char* szErr2K=NULL);

    ///����commit���͵Ķ��У��ύcommit��Ϣ��
    ///����ֵ��0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
    int commitBinlog(string const& strQueue,
        CWX_UINT64 ullSid,
        bool bCommit=true,
        char* szErr2K=NULL);
    ///��Ϣ������ϣ�bSend=true��ʾ�Ѿ����ͳɹ���false��ʾ����ʧ��
    ///����ֵ��0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
    int endSendMsg(string const& strQueue,
        CWX_UINT64 ullSid,
        bool bSend=true,
        char* szErr2K=NULL);

    ///ǿ��flush mq��log�ļ�
    void commit();
    ///���commit���Ͷ��г�ʱ����Ϣ
    void checkTimeout(CWX_UINT32 ttTimestamp);
    ///1���ɹ�
    ///0������
    ///-1����������
    int addQueue(string const& strQueue,
        CWX_UINT64 ullSid,
        bool bCommit,
        string const& strUser,
        string const& strPasswd,
        string const& strScribe,
        CWX_UINT32 uiDefTimeout,
        CWX_UINT32 uiMaxTimeout,
        char* szErr2K=NULL);
    ///1���ɹ�
    ///0��������
    ///-1����������
    int delQueue(string const& strQueue,
        char* szErr2K=NULL);

    void getQueuesInfo(list<CwxMqQueueInfo>& queues);

    inline bool isExistQueue(string const& strQueue)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        return m_queues.find(strQueue) != m_queues.end();
    }
    //-1��Ȩ��ʧ�ܣ�0�����в����ڣ�1���ɹ�
    inline int authQueue(string const& strQueue, string const& user, string const& passwd)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::const_iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return 0;
        if (iter->second->getUserName().length())
        {
            return ((user != iter->second->getUserName()) || (passwd != iter->second->getPasswd()))?-1:1;
        }
        return 1;
    }
    inline CWX_UINT32 getQueueNum()
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        return m_queues.size();
    }    
    inline bool isValid() const
    {
        return m_mqLogFile != NULL;
    }
    inline string const& getErrMsg() const
    {
        return m_strErrMsg;
    }

private:
    ///��������
    bool _save();

private:
    map<string, CwxMqQueue*>   m_queues;
    CwxRwLock                  m_lock;
    string                     m_strQueueLogFile;
    CWX_UINT32                 m_uiMaxFsyncNum;
    CwxMqQueueLogFile*         m_mqLogFile;
    CwxBinLogMgr*              m_binLog;
    CWX_UINT32                 m_uiLastSaveTime; ///<��һ��log�ļ��ı���ʱ��
    string                     m_strErrMsg;
};


#endif
