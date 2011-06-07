#include "CwxMqQueueMgr.h"

CwxMqQueue::CwxMqQueue(string strName,
                       string strUser,
                       string strPasswd,
                       bool    bCommit,
                       string strSubscribe,
                       CWX_UINT32 uiDefTimeout,
                       CWX_UINT32 uiMaxTimeout,
                       CwxBinLogMgr* pBinlog)
{
    m_strName = strName;
    m_strUser = strUser;
    m_strPasswd = strPasswd;
    m_bCommit = bCommit;
    m_uiDefTimeout = uiDefTimeout;
    m_uiMaxTimeout = uiMaxTimeout;
    m_strSubScribe = strSubscribe;
    m_binLog = pBinlog;
    m_pUncommitMsg =NULL;
    m_cursor = NULL;
}

CwxMqQueue::~CwxMqQueue()
{
    if (m_cursor) m_binLog->destoryCurser(m_cursor);
    if (m_memMsgMap.size())
    {
        map<CWX_UINT64, CwxMsgBlock*>::iterator iter = m_memMsgMap.begin();
        while(iter != m_memMsgMap.end())
        {
            CwxMsgBlockAlloc::free(iter->second);
            iter++;
        }
        m_memMsgMap.clear();
    }
    if (m_pUncommitMsg)
    {
        CwxMqQueueHeapItem* item=NULL;
        while((item = m_pUncommitMsg->pop()))
        {
            delete item;
        }
        delete m_pUncommitMsg;
    }
    if (!m_bCommit)
    {
        map<CWX_UINT64, void*>::iterator iter = m_uncommitMap.begin();
        while(iter != m_uncommitMap.end())
        {
            CwxMsgBlockAlloc::free((CwxMsgBlock*)iter->second);
            iter++;
        }
    }
    m_uncommitMap.clear();
}

int CwxMqQueue::init(CWX_UINT64 ullLastCommitSid,
                     set<CWX_UINT64> const& uncommitSid,
                     set<CWX_UINT64> const& commitSid,
                     string& strErrMsg)
{
    if (m_memMsgMap.size())
    {
        map<CWX_UINT64, CwxMsgBlock*>::iterator iter = m_memMsgMap.begin();
        while(iter != m_memMsgMap.end())
        {
            CwxMsgBlockAlloc::free(iter->second);
            iter++;
        }
        m_memMsgMap.clear();
    }
    if (m_cursor) m_binLog->destoryCurser(m_cursor);
    m_cursor = NULL;


    if (m_pUncommitMsg)
    {
        CwxMqQueueHeapItem* item=NULL;
        while((item = m_pUncommitMsg->pop()))
        {
            delete item;
        }
        delete m_pUncommitMsg;
        m_pUncommitMsg = NULL;
    }
    if (!m_bCommit)
    {
        map<CWX_UINT64, void*>::iterator iter = m_uncommitMap.begin();
        while(iter != m_uncommitMap.end())
        {
            CwxMsgBlockAlloc::free((CwxMsgBlock*)iter->second);
            iter++;
        }
    }
    m_uncommitMap.clear();

    if (m_bCommit)
    {
        m_pUncommitMsg = new CwxMinHeap<CwxMqQueueHeapItem>(2048);
        if (0 != m_pUncommitMsg->init())
        {
            strErrMsg = "Failure to init min-heap for no memory.";
            return -1;
        }
    }
    if (!CwxMqPoco::parseSubsribe(m_strSubScribe, m_subscribe, strErrMsg))
    {
        return -1;
    }

    m_ullLastCommitSid = ullLastCommitSid; ///<��־�ļ���¼��cursor��sid
    m_lastUncommitSid = uncommitSid; ///<m_ullLastCommitSid֮ǰδcommit��binlog
    m_lastCommitSid = commitSid;

    return 0;
}

///0��û����Ϣ��
///1����ȡһ����Ϣ��
///2���ﵽ�������㣬��û�з�����Ϣ��
///-1��ʧ�ܣ�
int CwxMqQueue::getNextBinlog(CwxMqTss* pTss,
                              CwxMsgBlock*&msg,
                              CWX_UINT32 uiTimeout,
                              int& err_num,
                              char* szErr2K)
{
    int iRet = 0;
    msg =  NULL;
    if (m_bCommit)
    {
        if (uiTimeout == 0) uiTimeout = m_uiDefTimeout;
        if (uiTimeout > m_uiMaxTimeout) uiTimeout = m_uiMaxTimeout;
    }

    if (m_memMsgMap.size())
    {
        msg = m_memMsgMap.begin()->second;
        m_memMsgMap.erase(m_memMsgMap.begin());
    }
    else
    {
        iRet = fetchNextBinlog(pTss, msg, err_num, szErr2K);
        if (1 != iRet) return iRet;
    }
    
    if (m_bCommit)
    {
        CWX_UINT32 uiTimestamp = time(NULL);
        uiTimestamp += uiTimeout;
        CwxMqQueueHeapItem* item = new CwxMqQueueHeapItem();
        item->msg(msg);
        item->timestamp(uiTimestamp);
        item->sid(msg->event().m_ullArg);
        item->send(false);
        m_uncommitMap[item->sid()] = item;
        m_pUncommitMsg->push(item);
    }
    else
    {
        m_uncommitMap[msg->event().m_ullArg] = msg;
    }
    return 1;
}

///����commit���͵Ķ��У��ύcommit��Ϣ��
///����ֵ��0�������ڣ�1���ɹ�.
int CwxMqQueue::commitBinlog(CWX_UINT64 ullSid, bool bCommit)
{
    if (!m_bCommit) return 0;
    map<CWX_UINT64, void*>::iterator iter=m_uncommitMap.find(ullSid);
    if (iter == m_uncommitMap.end()) return 0;
    ///�����ǵ��̻߳�������ʱ����Ϣһ��û�г�ʱ
    CwxMqQueueHeapItem* item = (CwxMqQueueHeapItem*)iter->second;
    CWX_ASSERT(-1 != item->index());
    //�Ӷ���ɾ��Ԫ��
    m_pUncommitMsg->erase(item);
    //��uncommit map��ɾ��Ԫ��
    m_uncommitMap.erase(iter);
    if (!bCommit)
    {
        m_memMsgMap[item->sid()] = item->msg();
        item->msg(NULL);
    }
    //ɾ��Ԫ������ͬʱ
    delete item;
    return 1;
}

///��Ϣ������ϣ�bSend=true��ʾ�Ѿ����ͳɹ���false��ʾ����ʧ��
///����ֵ��0�������ڣ�1���ɹ�.
int CwxMqQueue::endSendMsg(CWX_UINT64 ullSid, bool bSend)
{
    map<CWX_UINT64, void*>::iterator iter=m_uncommitMap.find(ullSid);
    if (iter == m_uncommitMap.end()) return 0;
    if (m_bCommit)
    {
        CwxMqQueueHeapItem* item = (CwxMqQueueHeapItem*)iter->second;
        if (-1 == item->index())
        {///��Ϣ�Ѿ���ʱ
            ///��δcommit map��ɾ����Ϣ
            m_uncommitMap.erase(iter);
            ///����Ϣ�ŵ��ڴ���Ϣmap
            m_memMsgMap[ullSid] = item->msg();
            item->msg(NULL);
            ///ɾ��item
            delete item;
        }
        else if (!bSend)
        {///��Ϣ����ʧ��
            ///��heap��ɾ����Ϣ
            m_pUncommitMsg->erase(item);///<��δ
            ///��δcommit map��ɾ����Ϣ
            m_uncommitMap.erase(iter);
            ///����Ϣ�ŵ��ڴ���Ϣmap
            m_memMsgMap[ullSid] = item->msg();
            item->msg(NULL);
            ///ɾ��item
            delete item;
        }
        else
        {///��Ϣ���ͳɹ�����û�г�ʱ
            item->send(true);
        }
    }
    else
    {
        CwxMsgBlock* msg = (CwxMsgBlock*)iter->second;
        if (bSend)
        {
            CwxMsgBlockAlloc::free(msg);
        }
        else
        {
            m_memMsgMap[ullSid] = msg;
        }
        m_uncommitMap.erase(iter);
    }
    return 1;
}

///���commit���Ͷ��г�ʱ����Ϣ
void CwxMqQueue::checkTimeout(CWX_UINT32 ttTimestamp)
{
    if (m_bCommit)
    {
        CwxMqQueueHeapItem * item = NULL;
        while(m_pUncommitMsg->count())
        {
            if (m_pUncommitMsg->top()->timestamp() > ttTimestamp) break;
            ///��Ϣ��ʱ
            item = m_pUncommitMsg->pop();
            if (item->send())
            {///��Ϣ�Ѿ��������
                ///��δ�ύmap��ɾ����Ϣ
                m_uncommitMap.erase(item->sid());
                ///����Ϣ�ŵ�δ���͵��ڴ�map��
                m_memMsgMap[item->sid()] = item->msg();
                item->msg(NULL);
                delete item;
            }
            else
            {///��Ϣ��û�з�����ϣ���ʱ��index��Ϊ-1����ʾ�Ѿ���ʱ
                item->index(-1);
            }
        }
    }
}

///0��û����Ϣ��
///1����ȡһ����Ϣ��
///2���ﵽ�������㣬��û�з�����Ϣ��
///-1��ʧ�ܣ�
int CwxMqQueue::fetchNextBinlog(CwxMqTss* pTss,
                    CwxMsgBlock*&msg,
                    int& err_num,
                    char* szErr2K)
{
    int iRet = 0;

    if (!m_cursor)
    {
        CWX_UINT64 ullStartSid = getStartSid();
        if (ullStartSid < m_binLog->getMaxSid())
        {
            m_cursor = m_binLog->createCurser();
            if (!m_cursor)
            {
                err_num = CWX_MQ_ERR_INNER_ERR;
                strcpy(szErr2K, "Failure to create cursor");
                return -1;
            }
            iRet = m_binLog->seek(m_cursor, ullStartSid);
            if (1 != iRet)
            {
                if (-1 == iRet)
                {
                    strcpy(szErr2K, m_cursor->getErrMsg());
                }
                else
                {
                    strcpy(szErr2K, "Binlog's seek should return 1 but zero");
                }
                m_binLog->destoryCurser(m_cursor);
                m_cursor = NULL;
                err_num = CWX_MQ_ERR_INNER_ERR;
                return -1;
            }
            if (ullStartSid ==m_cursor->getHeader().getSid())
            {
                iRet = m_binLog->next(m_cursor);
                if (0 == iRet) return 0; ///<����β��
                if (-1 == iRet)
                {///<ʧ��
                    strcpy(szErr2K, m_cursor->getErrMsg());
                    err_num = CWX_MQ_ERR_INNER_ERR;
                    return -1;
                }
            }
        }
        else
        {
            return 0;
        }
    }
    else
    {
        iRet = m_binLog->next(m_cursor);
        if (0 == iRet) return 0; ///<����β��
        if (-1 == iRet)
        {///<ʧ��
            strcpy(szErr2K, m_cursor->getErrMsg());
            err_num = CWX_MQ_ERR_INNER_ERR;
            return -1;
        }
    }
    CWX_UINT32 uiSkipNum = 0;
    bool bFetch = false;
    do 
    {
        while(!CwxMqPoco::isSubscribe(m_subscribe,
            false,
            m_cursor->getHeader().getGroup(),
            m_cursor->getHeader().getType()))
        {
            iRet = m_binLog->next(m_cursor);
            if (0 == iRet) return 0; ///<����β��
            if (-1 == iRet)
            {///<ʧ��
                strcpy(szErr2K, m_cursor->getErrMsg());
                err_num = CWX_MQ_ERR_INNER_ERR;
                return -1;
            }
            uiSkipNum ++;
            if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 2;
            continue;
        }
        bFetch = false;
        if (m_cursor->getHeader().getSid() <= m_ullLastCommitSid)
        {//ֻȡm_lastUncommitSid�е�����
            if (m_lastUncommitSid.size() && 
                (m_lastUncommitSid.find(m_cursor->getHeader().getSid()) != m_lastUncommitSid.end()))
            {
                bFetch = true;
                m_lastUncommitSid.erase(m_lastUncommitSid.find(m_cursor->getHeader().getSid()));
            }
        }
        else
        {//��ȡm_lastCommitSid���Ѿ�commit������
            if (!m_lastCommitSid.size()||
                (m_lastCommitSid.find(m_cursor->getHeader().getSid()) == m_lastCommitSid.end()))
            {
                bFetch = true;
            }
            else
            {
                m_lastCommitSid.erase(m_lastCommitSid.find(m_cursor->getHeader().getSid()));
            }
        }

        if (bFetch)
        {
            //fetch data
            ///��ȡbinlog��data����
            CWX_UINT32 uiDataLen = m_cursor->getHeader().getLogLen();
            ///׼��data��ȡ��buf
            char* pBuf = pTss->getBuf(uiDataLen);        
            ///��ȡdata
            iRet = m_binLog->fetch(m_cursor, pBuf, uiDataLen);
            if (-1 == iRet)
            {//��ȡʧ��
                strcpy(szErr2K, m_cursor->getErrMsg());
                err_num = CWX_MQ_ERR_INNER_ERR;
                return -1;
            }
            ///unpack data�����ݰ�
            if (pTss->m_pReader->unpack(pBuf, uiDataLen, false, true))
            {
                ///��ȡCWX_MQ_DATA��key����Ϊ����data����
                CwxKeyValueItem const* pItem = pTss->m_pReader->getKey(CWX_MQ_DATA);
                if (pItem)
                {
                    ///�γ�binlog���͵����ݰ�
                    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packFetchMqReply(pTss->m_pWriter,
                        msg,
                        CWX_MQ_ERR_SUCCESS,
                        "",
                        m_cursor->getHeader().getSid(),
                        m_cursor->getHeader().getDatetime(),
                        *pItem,
                        m_cursor->getHeader().getGroup(),
                        m_cursor->getHeader().getType(),
                        m_cursor->getHeader().getAttr(),
                        pTss->m_szBuf2K))
                    {
                        ///�γ����ݰ�ʧ��
                        err_num = CWX_MQ_ERR_INNER_ERR;
                        return -1;
                    }
                    else
                    {
                        msg->event().m_ullArg = m_cursor->getHeader().getSid();
                        err_num = CWX_MQ_ERR_SUCCESS;
                        return 1;
                    }
                }
                else
                {///��ȡ��������Ч
                    char szBuf[64];
                    CWX_ERROR(("Can't find key[%s] in binlog, sid=%s", CWX_MQ_DATA,
                        CwxCommon::toString(m_cursor->getHeader().getSid(), szBuf)));
                }            
            }
            else
            {///binlog�����ݸ�ʽ���󣬲���kv
                char szBuf[64];
                CWX_ERROR(("Can't unpack binlog, sid=%s",
                    CwxCommon::toString(m_cursor->getHeader().getSid(), szBuf)));
            }
        }
        uiSkipNum ++;
        if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 2;
        iRet = m_binLog->next(m_cursor);
        if (0 == iRet) return 0; ///<����β��
        if (-1 == iRet)
        {///<ʧ��
            strcpy(szErr2K, m_cursor->getErrMsg());
            err_num = CWX_MQ_ERR_INNER_ERR;
            return -1;
        }
    }while(1);
    return 0;
}

CWX_UINT64 CwxMqQueue::getMqNum()
{
    CWX_UINT64 ullStartSid = getStartSid();
    if (!m_cursor)
    {
        if (ullStartSid < m_binLog->getMaxSid())
        {
            m_cursor = m_binLog->createCurser();
            if (!m_cursor)
            {
                return 0;
            }
            int iRet = m_binLog->seek(m_cursor, ullStartSid);
            if (1 != iRet)
            {
                m_binLog->destoryCurser(m_cursor);
                m_cursor = NULL;
                return 0;
            }
        }
        return 0;
    }
    return m_binLog->leftLogNum(m_cursor) + m_memMsgMap.size();
}

void CwxMqQueue::getQueueDumpInfo(CWX_UINT64& ullLastCommitSid,
                      set<CWX_UINT64>& uncommitSid,
                      set<CWX_UINT64>& commitSid)
{
    if (m_cursor && (CwxBinLogMgr::CURSOR_STATE_READY == m_cursor->getSeekState()))
    {///cursor��Ч����ʱ��m_lastUncommitSid��С��cursor sid�ļ�¼Ӧ��ɾ��
        ///ԭ���ǣ�1�����м�¼�Ѿ�ʧЧ��2�����ڴ��uncommit�м�¼��
        set<CWX_UINT64>::iterator iter = m_lastUncommitSid.begin();
        while(iter != m_lastUncommitSid.end())
        {
            if (*iter >= m_cursor->getHeader().getSid()) break;
            m_lastUncommitSid.erase(iter);
            iter = m_lastUncommitSid.begin();
        }
        ///m_lastCommitSid�У�С��cursor sid�ļ�¼Ӧ��ɾ��
        ///��Ϊ���¼����cursor sid���commit��¼��
        iter = m_lastCommitSid.begin();
        while(iter != m_lastCommitSid.end())
        {
            if (*iter >= m_cursor->getHeader().getSid()) break;
            m_lastCommitSid.erase(iter);
            iter = m_lastCommitSid.begin();
        }
        ullLastCommitSid = m_cursor->getHeader().getSid();
    }
    else
    {
        ullLastCommitSid = m_ullLastCommitSid;
    }
    {//�γ�δcommit��sid
        uncommitSid.clear();
        //���m_lastUncommitSid�еļ�¼
        uncommitSid = m_lastUncommitSid;
        //���m_uncommitMap�еļ�¼
        {
            map<CWX_UINT64, void*>::iterator iter = m_uncommitMap.begin();
            while(iter != m_uncommitMap.end())
            {
                uncommitSid.insert(iter->first);
                iter++;
            }
        }
        //���m_memMsgMap�еļ�¼
        {
            map<CWX_UINT64, CwxMsgBlock*>::iterator iter = m_memMsgMap.begin();
            while(iter != m_memMsgMap.end())
            {
                uncommitSid.insert(iter->first);
                iter++;
            }
        }
    }
    {///�γ�commit�ļ�¼
        commitSid.clear();
        commitSid = m_lastCommitSid;
    }
}

CwxMqQueueMgr::CwxMqQueueMgr(string const& strQueueLogFile,
                             CWX_UINT32 uiMaxFsyncNum)
{
    m_strQueueLogFile = strQueueLogFile;
    m_uiMaxFsyncNum = uiMaxFsyncNum;
    m_mqLogFile = NULL;
    m_binLog = NULL;
    m_strErrMsg = "Not init";
}

CwxMqQueueMgr::~CwxMqQueueMgr()
{
    map<string, CwxMqQueue*>::iterator iter =  m_queues.begin();
    while(iter != m_queues.end())
    {
        delete iter->second;
        iter++;
    }
    if (m_mqLogFile) delete m_mqLogFile;
}

int CwxMqQueueMgr::init(CwxBinLogMgr* binLog)
{
    CwxMqQueue* mq = NULL;
    m_uiLastSaveTime = time(NULL);
    if (m_mqLogFile) delete m_mqLogFile;
    m_binLog = binLog;
    m_mqLogFile = new CwxMqQueueLogFile(m_uiMaxFsyncNum, m_strQueueLogFile);
    map<string, CwxMqQueueInfo> queues;
    map<string, set<CWX_UINT64>*> uncommitSets;
    map<string, set<CWX_UINT64>*> commitSets;
    
    if (0 != m_mqLogFile->init(queues, uncommitSets, commitSets))
    {
        char szBuf[2048];
        CwxCommon::snprintf(szBuf, 2047, "Failure to init mq queue log-file, err:%s", m_mqLogFile->getErrMsg());
        m_strErrMsg = szBuf;
        delete m_mqLogFile;
        m_mqLogFile = NULL;
        return -1;
    }
    set<CWX_UINT64> empty;
    map<string, CwxMqQueueInfo>::iterator iter_queue = queues.begin();
    set<CWX_UINT64>* pUncommitSet = NULL;
    set<CWX_UINT64>* pCommitSet = NULL;
    do 
    {
        while(iter_queue != queues.end())
        {
            mq = new CwxMqQueue(iter_queue->second.m_strName, 
                iter_queue->second.m_strUser,
                iter_queue->second.m_strPasswd,
                iter_queue->second.m_bCommit,
                iter_queue->second.m_strSubScribe,
                iter_queue->second.m_uiDefTimeout,
                iter_queue->second.m_uiMaxTimeout,
                m_binLog);
            if (uncommitSets.find(iter_queue->second.m_strName) != uncommitSets.end())
            {
                pUncommitSet = uncommitSets[iter_queue->second.m_strName];
            }
            else
            {
                pUncommitSet = &empty;
            }
            if (commitSets.find(iter_queue->second.m_strName) != commitSets.end())
            {
                pCommitSet = commitSets[iter_queue->second.m_strName];
            }
            else
            {
                pCommitSet = &empty;
            }
            if (0 != mq->init(iter_queue->second.m_ullCursorSid, *pUncommitSet, *pCommitSet, m_strErrMsg))
            {
                break;
            }
            m_queues[mq->getName()] = mq;
            iter_queue ++;
        }
    } while(0);
    map<string, set<CWX_UINT64>*>::iterator iter = uncommitSets.begin();
    while(iter != uncommitSets.end())
    {
        delete iter->second;
        iter++;
    }
    iter = commitSets.begin();
    while(iter != commitSets.end())
    {
        delete iter->second;
        iter++;
    }
    if (iter_queue != queues.end())
    {
        delete m_mqLogFile;
        m_mqLogFile = NULL;
    }
    return iter_queue == queues.end()?0:-1;
}


///0��û����Ϣ��
///1����ȡһ����Ϣ��
///2���ﵽ�������㣬��û�з�����Ϣ��
///-1��ʧ�ܣ�
///-2�����в�����
int CwxMqQueueMgr::getNextBinlog(CwxMqTss* pTss,
                  string const& strQueue,
                  CwxMsgBlock*&msg,
                  CWX_UINT32 uiTimeout,
                  int& err_num,
                  bool& bCommitType, ///<�Ƿ�Ϊcommit���͵Ķ���
                  char* szErr2K)
{
    if (m_mqLogFile)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
        bCommitType = iter->second->isCommit();
        return iter->second->getNextBinlog(pTss, msg, uiTimeout, err_num, szErr2K);
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}

///����commit���͵Ķ��У��ύcommit��Ϣ��
///����ֵ��0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
int CwxMqQueueMgr::commitBinlog(string const& strQueue,
                 CWX_UINT64 ullSid,
                 bool bCommit,
                 char* szErr2K)
{
    if (m_mqLogFile)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
        int ret = iter->second->commitBinlog(ullSid, bCommit);
        if (0 == ret) return 0;
        if (1 == ret)
        {
            int num = m_mqLogFile->log(iter->second->getName().c_str(), ullSid);
            if (-1 == num)
            {
                m_strErrMsg = m_mqLogFile->getErrMsg();
                delete m_mqLogFile;
                m_mqLogFile = NULL;
                if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
                return -1;
            }
            if (num >= MQ_SWITCH_LOG_NUM)
            {
                if (!_save())
                {
                    delete m_mqLogFile;
                    m_mqLogFile = NULL;
                    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
                    return -1;
                }
            }
            return 1;
        }
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}

///��Ϣ������ϣ�bSend=true��ʾ�Ѿ����ͳɹ���false��ʾ����ʧ��
///����ֵ��0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
int CwxMqQueueMgr::endSendMsg(string const& strQueue,
               CWX_UINT64 ullSid,
               bool bSend,
               char* szErr2K)
{
    if (m_mqLogFile)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
        int ret = iter->second->endSendMsg(ullSid, bSend);
        if (0 == ret) return 0;
        if (1 == ret) 
        {
            if (!iter->second->isCommit())
            {
                int num = m_mqLogFile->log(iter->second->getName().c_str(), ullSid);
                if (-1 == num)
                {
                    m_strErrMsg = m_mqLogFile->getErrMsg();
                    delete m_mqLogFile;
                    m_mqLogFile = NULL;
                    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
                    return -1;
                }
                if (num >= MQ_SWITCH_LOG_NUM)
                {
                    if (!_save())
                    {
                        delete m_mqLogFile;
                        m_mqLogFile = NULL;
                        if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
                        return -1;
                    }
                }
            }
            return 1;
        }
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}


///ǿ��flush mq��log�ļ�
void CwxMqQueueMgr::commit()
{
    if (m_mqLogFile) m_mqLogFile->fsync();
}

///���commit���Ͷ��г�ʱ����Ϣ
void CwxMqQueueMgr::checkTimeout(CWX_UINT32 ttTimestamp)
{
    CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
    map<string, CwxMqQueue*>::iterator iter = m_queues.begin();
    while(iter != m_queues.end())
    {
        if (iter->second->isCommit()) iter->second->checkTimeout(ttTimestamp);
        iter++;
    }
    if (ttTimestamp > m_uiLastSaveTime  + MQ_MAX_SWITCH_LOG_INTERNAL)
    {
        if (!_save())
        {
            delete m_mqLogFile;
            m_mqLogFile = NULL;
        }
    }
}

///1���ɹ�
///0������
///-1����������
int CwxMqQueueMgr::addQueue(string const& strQueue,
                            CWX_UINT64 ullSid,
                            bool bCommit,
                            string const& strUser,
                            string const& strPasswd,
                            string const& strScribe,
                            CWX_UINT32 uiDefTimeout,
                            CWX_UINT32 uiMaxTimeout,
                            char* szErr2K)
{
    if (m_mqLogFile)
    {
        CwxWriteLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::iterator iter = m_queues.find(strQueue);
        if (iter != m_queues.end()) return 0;
        set<CWX_UINT64> empty;
        CwxMqQueue* mq = new CwxMqQueue(strQueue, 
            strUser,
            strPasswd,
            bCommit,
            strScribe,
            uiDefTimeout,
            uiMaxTimeout,
            m_binLog);
        string strErr;
        if (0 != mq->init(ullSid, empty, empty, strErr))
        {
            delete mq;
            return -1;
        }
        m_queues[strQueue] = mq;
        if (!_save())
        {
            delete m_mqLogFile;
            m_mqLogFile = NULL;
            if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
            return -1;
        }
        return 1;
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}
///1���ɹ�
///0��������
///-1����������
int CwxMqQueueMgr::delQueue(string const& strQueue,
             char* szErr2K)
{
    if (m_mqLogFile)
    {
        CwxWriteLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, CwxMqQueue*>::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return 0;
        delete iter->second;
        m_queues.erase(iter);
        if (!_save())
        {
            delete m_mqLogFile;
            m_mqLogFile = NULL;
            if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
            return -1;
        }
        return 1;
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}

void CwxMqQueueMgr::getQueuesInfo(list<CwxMqQueueInfo>& queues)
{
    CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
    CwxMqQueueInfo info;
    map<string, CwxMqQueue*>::const_iterator iter = m_queues.begin();
    while(iter != m_queues.end())
    {
        info.m_strName = iter->second->getName();
        info.m_strUser = iter->second->getUserName();
        info.m_bCommit = iter->second->isCommit();
        info.m_uiDefTimeout = iter->second->getDefTimeout();
        info.m_uiMaxTimeout = iter->second->getMaxTimeout();
        info.m_strSubScribe = iter->second->getSubscribeRule();
        info.m_ullCursorSid = iter->second->getCursorSid();
        info.m_ullLeftNum = iter->second->getMqNum();
        info.m_uiWaitCommitNum = iter->second->getWaitCommitNum();
        info.m_uiMemLogNum = iter->second->getMemMsgMap().size();
        if (iter->second->getCursor())
        {
            info.m_ucQueueState = iter->second->getCursor()->getSeekState();
            if (CwxBinLogMgr::CURSOR_STATE_ERROR == info.m_ucQueueState)
            {
                info.m_strQueueErrMsg = iter->second->getCursor()->getErrMsg();
            }
            else
            {
                info.m_strQueueErrMsg = "";
            }
        }
        else
        {
            info.m_ucQueueState = CwxBinLogMgr::CURSOR_STATE_UNSEEK;
            info.m_strQueueErrMsg = "";
        }
        queues.push_back(info);
        iter++;
    }
}

bool CwxMqQueueMgr::_save()
{
    if (m_mqLogFile)
    {
        map<string, CwxMqQueueInfo> queues;
        map<string, set<CWX_UINT64>*> uncommitSets;
        map<string, set<CWX_UINT64>*> commitSets;
        CwxMqQueueInfo queueInfo;
        set<CWX_UINT64>* sidUncommitSet=NULL;
        set<CWX_UINT64>* sidCommitSet =NULL;
        map<string, CwxMqQueue*>::iterator iter = m_queues.begin();
        while(iter != m_queues.end())
        {
            queueInfo.m_strName = iter->second->getName();
            queueInfo.m_strUser = iter->second->getUserName();
            queueInfo.m_strPasswd = iter->second->getPasswd();
            queueInfo.m_bCommit = iter->second->isCommit();
            queueInfo.m_uiDefTimeout = iter->second->getDefTimeout();
            queueInfo.m_uiMaxTimeout = iter->second->getMaxTimeout();
            queueInfo.m_strSubScribe = iter->second->getSubscribeRule();
            queueInfo.m_ullCursorSid = iter->second->getCursorSid();
            queues[queueInfo.m_strName] = queueInfo;

            if (!sidUncommitSet) sidUncommitSet = new set<CWX_UINT64>;
            if (!sidCommitSet) sidCommitSet = new set<CWX_UINT64>;

            iter->second->getQueueDumpInfo(queueInfo.m_ullCursorSid, *sidUncommitSet, *sidCommitSet);
            if (sidUncommitSet->size())
            {
                uncommitSets[queueInfo.m_strName] = sidUncommitSet;
                sidUncommitSet = NULL;
            }
            if (sidCommitSet->size())
            {
                commitSets[queueInfo.m_strName] = sidCommitSet;
                sidCommitSet = NULL;
            }
            iter++;
        }
        if (sidUncommitSet) delete  sidUncommitSet;
        if (sidCommitSet) delete sidCommitSet;

        //���浽log��
        if (0 != m_mqLogFile->save(queues, uncommitSets, commitSets))
        {
            m_strErrMsg = m_mqLogFile->getErrMsg();
            return false;
        }
        //���map
        map<string, set<CWX_UINT64>*>::iterator iter_sid = uncommitSets.begin();
        while(iter_sid != uncommitSets.end())
        {
            delete iter_sid->second;
            iter_sid ++;
        }
        iter_sid = commitSets.begin();
        while(iter_sid != commitSets.end())
        {
            delete iter_sid->second;
            iter_sid ++;
        }
        m_uiLastSaveTime = time(NULL);
        return true;
    }
    return false;
}
