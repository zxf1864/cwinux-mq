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
    m_pDelayMsg = NULL;
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
    if (m_pDelayMsg)
    {
        CwxMqQueueHeapItem* item=NULL;
        while((item = m_pDelayMsg->pop()))
        {
            delete item;
        }
        delete m_pDelayMsg;
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
    if (m_pDelayMsg)
    {
        CwxMqQueueHeapItem* item=NULL;
        while((item = m_pDelayMsg->pop()))
        {
            delete item;
        }
        delete m_pDelayMsg;
        m_pDelayMsg = NULL;
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
            strErrMsg = "Failure to init uncommit min-heap for less memory.";
            return -1;
        }
        m_pDelayMsg = new CwxMinHeap<CwxMqQueueHeapItem>(2048);
        if (0 != m_pDelayMsg->init())
        {
            strErrMsg = "Failure to init delay min-heap for less memory.";
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

	//����cursor
	{
		m_cursor = m_binLog->createCurser(getStartSid());
		if (!m_cursor)
		{
			strErrMsg = "Failure to create cursor.";
			return -1;
		}
	}
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
int CwxMqQueue::commitBinlog(CWX_UINT64 ullSid,
                             bool bCommit,
                             CWX_UINT32 uiDeley)
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
    {//��������ύ����
        if (!uiDeley)
        {
            m_memMsgMap[item->sid()] = item->msg();
            item->msg(NULL);
        }
        else
        {
            item->index(-1);
            item->timestamp(((CWX_UINT32)time(NULL)) + uiDeley);
            m_pDelayMsg->push(item);
            item = NULL;
        }
    }
    //ɾ��Ԫ������ͬʱ
    if (item) delete item;
    return 1;
}

///��Ϣ������ϣ�bSend=true��ʾ�Ѿ����ͳɹ���false��ʾ����ʧ��
///����ֵ��0�������ڣ�1���ɹ�.
int CwxMqQueue::endSendMsg(CWX_UINT64 ullSid, bool bSend)
{
    map<CWX_UINT64, void*>::iterator iter=m_uncommitMap.find(ullSid);
    if (iter == m_uncommitMap.end()) return 0; ///�������uncommit Map�д��ڣ�˵��ullSid�����ڻ��Ѿ���ʱ
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
            ///��uncommit heap��ɾ����Ϣ
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
        {///���ͳɹ�
            CwxMsgBlockAlloc::free(msg);
        }
        else
        {///����ʧ�ܣ���Ҫ����������Ϣ
            m_memMsgMap[ullSid] = msg;
        }
        ///��δcommit��map��ɾ��
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
        ///��ⳬʱ��Ϣ
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
        ///��ⳬ��delayʱ��������Ϣ
        while(m_pDelayMsg->count())
        {
            if (m_pDelayMsg->top()->timestamp() > ttTimestamp) break;
            ///��Ϣ��ʱ
            item = m_pDelayMsg->pop();
            ///����Ϣ�ŵ�δ���͵��ڴ�map��
            m_memMsgMap[item->sid()] = item->msg();
            item->msg(NULL);
            delete item;
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

	if (!m_cursor || (CwxBinLogMgr::CURSOR_STATE_READY != m_cursor->getSeekState()))
    {
        CWX_UINT64 ullStartSid = getStartSid();
        //
        if (ullStartSid < m_binLog->getMaxSid())
        {
			if (!m_cursor)
			{
				m_cursor = m_binLog->createCurser();
				if (!m_cursor)
				{
					err_num = CWX_MQ_ERR_INNER_ERR;
					strcpy(szErr2K, "Failure to create cursor");
					return -1;
				}
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
    if (!m_cursor || (CwxBinLogMgr::CURSOR_STATE_READY != m_cursor->getSeekState()))
    {
        CWX_UINT64 ullStartSid = getStartSid();
        if (ullStartSid < m_binLog->getMaxSid())
        {
			if (!m_cursor)
			{
				m_cursor = m_binLog->createCurser();
				if (!m_cursor)
				{
					return 0;
				}
			}
            int iRet = m_binLog->seek(m_cursor, ullStartSid);
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
    return m_binLog->leftLogNum(m_cursor) + m_memMsgMap.size() + (m_pDelayMsg?m_pDelayMsg->count():0);
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
        ///�����ǰcursorû���Ƶ�m_ullLastCommitSid��λ�ã�
        ///��Ȼ����m_ullLastCommitSidΪcursor��λ�á�
        if (ullLastCommitSid < m_ullLastCommitSid) ullLastCommitSid = m_ullLastCommitSid;
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
        //���delay����Ϣ
        if(m_pDelayMsg)
        {
            for (CWX_UINT32 i=0; i<m_pDelayMsg->count(); i++)
            {
                uncommitSid.insert(m_pDelayMsg->at(i)->sid());
            }            
        }
    }
    {///�γ�commit�ļ�¼
        commitSid.clear();
        commitSid = m_lastCommitSid;
    }
}

CwxMqQueueMgr::CwxMqQueueMgr(string const& strQueueLogFilePath,
                             CWX_UINT32 uiMaxFsyncNum)
{
    m_strQueueLogFilePath = strQueueLogFilePath;
	if (m_strQueueLogFilePath[m_strQueueLogFilePath.length()-1] != '/')
		m_strQueueLogFilePath += "/";
    m_uiMaxFsyncNum = uiMaxFsyncNum;
    m_binLog = NULL;
    m_strErrMsg = "Not init";
	m_bValid = false;
}

CwxMqQueueMgr::~CwxMqQueueMgr()
{
    map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter =  m_queues.begin();
    while(iter != m_queues.end())
    {
        delete iter->second.first;
		delete iter->second.second;
        iter++;
    }
}

int CwxMqQueueMgr::init(CwxBinLogMgr* binLog)
{
    CwxMqQueue* mq = NULL;
	CwxMqQueueLogFile* mqLogFile = NULL;
	string  strMqLogFile;
    m_binLog = binLog;

	//�������
	{
		map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter =  m_queues.begin();
		while(iter != m_queues.end())
		{
			delete iter->second.first;
			delete iter->second.second;
			iter++;
		}
		m_queues.clear();
	}

	//��ʼ������
	{
		pair<CwxMqQueue*, CwxMqQueueLogFile*> mq_pair;
		CwxMqQueueInfo queue;
		string strQueueFile;
		string strQueuePathFile;
		set<CWX_UINT64> uncommitSets;
		set<CWX_UINT64> commitSets;
		set<string/*queue name*/ > queues;
		set<string/*queue name*/ >::iterator iter;
		if (!_fetchLogFile(queues)) return -1;
		iter  = queues.begin();
		while(iter != queues.end())
		{
			strQueuePathFile = m_strQueueLogFilePath + _getQueueLogFile(*iter, strQueueFile);
			mqLogFile = new CwxMqQueueLogFile(m_uiMaxFsyncNum, strQueuePathFile);
			queue.m_strName.erase();
			uncommitSets.clear();
			commitSets.clear();
			if (0 != mqLogFile->init(queue, uncommitSets, commitSets))
			{
				char szBuf[2048];
				CwxCommon::snprintf(szBuf, 2047, "Failure to init mq queue log-file:%s, err:%s",
					strQueuePathFile.c_str(),
					mqLogFile->getErrMsg());
				m_strErrMsg = szBuf;
				m_bValid = false;
				delete mqLogFile;
				return -1;
			}
			if (queue.m_strName.empty())
			{//�ն����ļ���ɾ��
				delete mqLogFile;
				CwxMqQueueLogFile::removeFile(strQueuePathFile);
				iter++;
				continue;
			}
			if (*iter != queue.m_strName)
			{
				char szBuf[2048];
				CwxCommon::snprintf(szBuf, 2047, "queue log file[%s]'s queue name should be [%s], but it's [%s]",
					strQueuePathFile.c_str(),
					iter->c_str(),
					queue.m_strName.c_str());
				m_strErrMsg = szBuf;
				m_bValid = false;
				delete mqLogFile;
				return -1;
			}
			mq = new CwxMqQueue(queue.m_strName, 
				queue.m_strUser,
				queue.m_strPasswd,
				queue.m_bCommit,
				queue.m_strSubScribe,
				queue.m_uiDefTimeout,
				queue.m_uiMaxTimeout,
				m_binLog);
			if (0 != mq->init(queue.m_ullCursorSid, uncommitSets, commitSets, m_strErrMsg))
			{
				delete mqLogFile;
				delete mq;
				m_bValid = false;
				return -1;
			}
			mq_pair.first = mq;
			mq_pair.second = mqLogFile;
			m_queues[queue.m_strName] = mq_pair;
		}
	}

	return 0;
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
    if (m_bValid)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
		if (!iter->second.second->isValid())
		{
			if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
			return -1;
		}
        bCommitType = iter->second.first->isCommit();
        return iter->second.first->getNextBinlog(pTss, msg, uiTimeout, err_num, szErr2K);
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}

///����commit���͵Ķ��У��ύcommit��Ϣ��
///����ֵ��0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
int CwxMqQueueMgr::commitBinlog(string const& strQueue,
                 CWX_UINT64 ullSid,
                 bool bCommit,
                 CWX_UINT32 uiDeley,
                 char* szErr2K)
{
    if (m_bValid)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
		map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
		if (!iter->second.second->isValid())
		{
			if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
			return -1;
		}
        int ret = iter->second.first->commitBinlog(ullSid, bCommit, uiDeley>CWX_MQ_MAX_TIMEOUT_SECOND?CWX_MQ_MAX_TIMEOUT_SECOND:uiDeley);
        if (0 == ret) return 0;
        if (1 == ret)
        {
            if (!bCommit) return 1; ///�������commit��ֱ�ӷ���1������¼commit��¼��
            int num = iter->second.second->log(ullSid);
            if (-1 == num)
            {
                if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
                return -1;
            }
            if (num >= MQ_SWITCH_LOG_NUM)
            {
                if (!_save(iter->second.first, iter->second.second))
                {
                    if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
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
    if (m_bValid)
    {
        CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return -2;
		if (!iter->second.second->isValid())
		{
			if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
			return -1;
		}
        int ret = iter->second.first->endSendMsg(ullSid, bSend);
        if (0 == ret) return 0;
        if (1 == ret) 
        {
            if (!iter->second.first->isCommit())
            {
                int num = iter->second.second->log(ullSid);
                if (-1 == num)
                {
                    if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
                    return -1;
                }
                if (num >= MQ_SWITCH_LOG_NUM)
                {
					if (!_save(iter->second.first, iter->second.second))
					{
						if (szErr2K) strcpy(szErr2K, iter->second.second->getErrMsg());
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
	if (m_bValid)
	{
		map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.begin();
		while(iter != m_queues.end())
		{
			iter->second.second->fsync();
			iter++;
		}
	}
}

///���commit���Ͷ��г�ʱ����Ϣ
void CwxMqQueueMgr::checkTimeout(CWX_UINT32 ttTimestamp)
{
    CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
	if (m_bValid)
	{
		map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.begin();
		while(iter != m_queues.end())
		{
			if (iter->second.second->isValid())
			{

				if (iter->second.first->isCommit()) iter->second.first->checkTimeout(ttTimestamp);
				if (ttTimestamp > iter->second.second->getLastSaveTime()  + MQ_MAX_SWITCH_LOG_INTERNAL)
				{
					if (!_save(iter->second.first, iter->second.second))
					{
					}
				}
			}
			iter++;
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
    if (m_bValid)
    {
        CwxWriteLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.find(strQueue);
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
			if (szErr2K) strcpy(szErr2K, strErr.c_str());
            return -1;
        }
		//create mq log file
		string strQueueFile;
		string strQueuePathFile;
		CwxMqQueueInfo queue;
		strQueuePathFile = m_strQueueLogFilePath + _getQueueLogFile(strQueue, strQueueFile);
		CwxMqQueueLogFile::removeFile(strQueuePathFile);
		CwxMqQueueLogFile* mqLogFile = new CwxMqQueueLogFile(m_uiMaxFsyncNum, strQueuePathFile);
		queue.m_strName.erase();
		if (0 != mqLogFile->init(queue, empty, empty))
		{
			char szBuf[2048];
			CwxCommon::snprintf(szBuf, 2047, "Failure to init mq queue log-file:%s, err:%s",
				strQueuePathFile.c_str(),
				mqLogFile->getErrMsg());
			delete mqLogFile;
			delete mq;
			if (szErr2K) strcpy(szErr2K, szBuf);
			return -1;
		}
        if (!_save(mq, mqLogFile))
        {
			char szBuf[2048];
			CwxCommon::snprintf(szBuf, 2047, "Failure to save mq queue log-file:%s, err:%s",
				strQueuePathFile.c_str(),
				mqLogFile->getErrMsg());
			delete mqLogFile;
			delete mq;
			if (szErr2K) strcpy(szErr2K, szBuf);
            return -1;
        }
		pair<CwxMqQueue*, CwxMqQueueLogFile*> item;
		item.first = mq;
		item.second = mqLogFile;
		m_queues[strQueue] = item;
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
    if (m_bValid)
    {
        CwxWriteLockGuard<CwxRwLock>  lock(&m_lock);
        map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::iterator iter = m_queues.find(strQueue);
        if (iter == m_queues.end()) return 0;
        delete iter->second.first;
		delete iter->second.second;
        m_queues.erase(iter);
		string strQueueFile;
		string strQueuePathFile;
		_getQueueLogFile(strQueue, strQueueFile);
		strQueuePathFile = m_strQueueLogFilePath + strQueueFile;
		CwxMqQueueLogFile::removeFile(strQueuePathFile);
        return 1;
    }
    if (szErr2K) strcpy(szErr2K, m_strErrMsg.c_str());
    return -1;
}

void CwxMqQueueMgr::getQueuesInfo(list<CwxMqQueueInfo>& queues)
{
    CwxReadLockGuard<CwxRwLock>  lock(&m_lock);
    CwxMqQueueInfo info;
    map<string, pair<CwxMqQueue*, CwxMqQueueLogFile*> >::const_iterator iter = m_queues.begin();
    while(iter != m_queues.end())
    {
        info.m_strName = iter->second.first->getName();
        info.m_strUser = iter->second.first->getUserName();
        info.m_bCommit = iter->second.first->isCommit();
        info.m_uiDefTimeout = iter->second.first->getDefTimeout();
        info.m_uiMaxTimeout = iter->second.first->getMaxTimeout();
        info.m_strSubScribe = iter->second.first->getSubscribeRule();
        info.m_ullCursorSid = iter->second.first->getCursorSid();
        info.m_ullLeftNum = iter->second.first->getMqNum();
        info.m_uiWaitCommitNum = iter->second.first->getWaitCommitNum();
        info.m_uiMemLogNum = iter->second.first->getMemMsgMap().size();
        if (iter->second.first->getCursor())
        {
            info.m_ucQueueState = iter->second.first->getCursor()->getSeekState();
            if (CwxBinLogMgr::CURSOR_STATE_ERROR == info.m_ucQueueState)
            {
                info.m_strQueueErrMsg = iter->second.first->getCursor()->getErrMsg();
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
		info.m_bQueueLogFileValid = iter->second.second->isValid();
		if (!info.m_bQueueLogFileValid)
		{
			info.m_strQueueLogFileErrMsg = iter->second.second->getErrMsg();
		}
		else
		{
			info.m_strQueueLogFileErrMsg.erase();
		}
        queues.push_back(info);
        iter++;
    }
}

bool CwxMqQueueMgr::_save(CwxMqQueue* queue, CwxMqQueueLogFile* logFile)
{
	CwxMqQueueInfo queueInfo;
	set<CWX_UINT64> uncommitSets;
	set<CWX_UINT64> commitSets;
	queueInfo.m_strName = queue->getName();
	queueInfo.m_strUser = queue->getUserName();
	queueInfo.m_strPasswd = queue->getPasswd();
	queueInfo.m_bCommit = queue->isCommit();
	queueInfo.m_uiDefTimeout = queue->getDefTimeout();
	queueInfo.m_uiMaxTimeout = queue->getMaxTimeout();
	queueInfo.m_strSubScribe = queue->getSubscribeRule();
	queueInfo.m_ullCursorSid = queue->getCursorSid();
	queue->getQueueDumpInfo(queueInfo.m_ullCursorSid, uncommitSets, commitSets);
	//���浽log��
	if (0 != logFile->save(queueInfo, uncommitSets, commitSets))
	{
		return false;
	}
	return true;
}

bool CwxMqQueueMgr::_fetchLogFile(set<string/*queue name*/> & queues)
{
	//���binlog��Ŀ¼�����ڣ��򴴽���Ŀ¼
	if (!CwxFile::isDir(m_strQueueLogFilePath.c_str()))
	{
		if (!CwxFile::createDir(m_strQueueLogFilePath.c_str()))
		{
			char szBuf[2048];
			CwxCommon::snprintf(szBuf, 2047, "Failure to create mq log path:%s, errno=%d", m_strQueueLogFilePath.c_str(), errno);
			m_strErrMsg = szBuf;
			m_bValid = false;
			return false;
		}
	}
	//��ȡĿ¼�µ������ļ�
	list<string> pathfiles;
	if (!CwxFile::getDirFile(m_strQueueLogFilePath, pathfiles))
	{
		char szBuf[2048];
		CwxCommon::snprintf(szBuf, 2047, "Failure to fetch mq log, path:%s, errno=%d", m_strQueueLogFilePath.c_str(), errno);
		m_strErrMsg = szBuf;
		m_bValid = false;
		return false;
	}
	//��ȡĿ¼�µ�����binlog�ļ������ŵ�map�У�����map������������ļ�
	string strQueue;
	list<string>::iterator iter=pathfiles.begin();
	queues.clear();
	while(iter != pathfiles.end())
	{
		if (_isQueueLogFile(*iter, strQueue))
		{
			queues.insert(strQueue);
		}
		iter++;
	}
	return true;
}

bool CwxMqQueueMgr::_isQueueLogFile(string const& file, string& queue)
{
	string strFile;
	list<string> items;
	list<string>::iterator iter;
	CwxCommon::split(file, items, '.');
	if ((3 != items.size())&&(4 != items.size())) return false;
	iter = items.begin();
	if (*iter != "queue_log") return false;
	iter++;
	queue = *iter;
	if (!isInvalidQueueName(queue.c_str())) return false;
	iter++;
	if (*iter != "log") return false;
	return true;
}

string& CwxMqQueueMgr::_getQueueLogFile(string const& queue, string& strFile)
{
	strFile = "queue_log.";
	strFile += queue;
	strFile += ".log";
	return strFile;
}
