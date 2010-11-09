#include "CwxMqQueueMgr.h"

CwxMqQueue::CwxMqQueue(CWX_UINT32 uiId,
                       string strName,
                       string strUser,
                       string strPasswd,
                       CWX_UINT64 ullStartSid,
                       CwxBinLogMgr* pBinlog)
{
    m_uiId = uiId;
    m_strName = strName;
    m_strUser = strUser;
    m_strPasswd = strPasswd;
    m_ullStartSid = ullStartSid;
    m_cursor = NULL;
    m_memMsgTail = new CwxSTail<CwxMsgBlock>;
    m_binLog = pBinlog;
}

CwxMqQueue::~CwxMqQueue()
{
    if (m_cursor) m_binLog->destoryCurser(m_cursor);
    if (m_memMsgTail->count())
    {
        CwxMsgBlock* msg = NULL;
        while((msg=m_memMsgTail->pop_head()))
        {
            CwxMsgBlockAlloc::free(msg);
        }
    }
    delete m_memMsgTail;
}
///0��û����Ϣ��1����ȡһ����Ϣ��
//2���ﵽ�������㣬��û�з�����Ϣ�� -1��ʧ�ܣ�
int CwxMqQueue::getNextBinlog(CwxMqTss* pTss,
                              bool bSync,
                              CwxMsgBlock*&msg,
                              int& err_num,
                              bool& bClose)
{
    int iRet = 0;
    err_num = CWX_MQ_SUCCESS;
    bClose = false;
    if (m_memMsgTail && m_memMsgTail->count())
    {
        msg = m_memMsgTail->pop_head();
        return 1;
    }
    if (!m_cursor)
    {
        if (m_ullStartSid < m_binLog->getMaxSid())
        {
            m_cursor = m_binLog->createCurser();
            if (!m_cursor)
            {
                err_num = CWX_MQ_INNER_ERR;
                strcpy(pTss->m_szBuf2K, "Failure to create cursor");
                bClose = true;
                return -1;
            }
            iRet = m_binLog->seek(m_cursor, m_ullStartSid);
            if (1 != iRet)
            {
                if (-1 == iRet)
                {
                    strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
                }
                else
                {
                    strcpy(pTss->m_szBuf2K, "Binlog's seek should return 1 but zero");
                }
                m_binLog->destoryCurser(m_cursor);
                m_cursor = NULL;
                err_num = CWX_MQ_INNER_ERR;
                bClose = true;
                return -1;
            }
            if (m_ullStartSid ==m_cursor->getHeader().getSid())
            {
                iRet = m_binLog->next(m_cursor);
                if (0 == iRet) return 0; ///<����β��
                if (-1 == iRet)
                {///<ʧ��
                    strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
                    err_num = CWX_MQ_INNER_ERR;
                    bClose = true;
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
            strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
            err_num = CWX_MQ_INNER_ERR;
            bClose = true;
            return -1;
        }
    }
    do 
    {
        CWX_UINT32 uiSkipNum = 0;
        while(!CwxMqPoco::isSubscribe(m_subscribe,
            bSync,
            m_cursor->getHeader().getGroup(),
            m_cursor->getHeader().getType()))
        {
            iRet = m_binLog->next(m_cursor);
            if (0 == iRet) return 0; ///<����β��
            if (-1 == iRet)
            {///<ʧ��
                strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
                err_num = CWX_MQ_INNER_ERR;
                bClose = true;
                return -1;
            }
            uiSkipNum ++;
            if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 2;
            continue;
        }
        //fetch data
        ///��ȡbinlog��data����
        CWX_UINT32 uiDataLen = m_cursor->getHeader().getLogLen();
        ///׼��data��ȡ��buf
        char* pBuf = pTss->getBuf(uiDataLen);        
        ///��ȡdata
        iRet = m_binLog->fetch(m_cursor, pBuf, uiDataLen);
        if (-1 == iRet)
        {//��ȡʧ��
            strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
            err_num = CWX_MQ_INNER_ERR;
            bClose = true;
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
                if (CWX_MQ_SUCCESS != CwxMqPoco::packFetchMqReply(pTss,
                    msg,
                    CWX_MQ_SUCCESS,
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
                    err_num = CWX_MQ_INNER_ERR;
                    return -1;
                }
                else
                {
                    msg->event().m_ullArg = m_cursor->getHeader().getSid();
                    msg->event().m_uiArg = m_uiId;
                    err_num = CWX_MQ_SUCCESS;
                    return 1;
                }
            }
            else
            {///��ȡ��������Ч
                char szBuf[64];
                CwxCommon::snprintf(pTss->m_szBuf2K,
                    2047,
                    "Can't find key[%s] in binlog, sid=%s", CWX_MQ_DATA,
                    CwxCommon::toString(m_cursor->getHeader().getSid(), szBuf));
            }            
        }
        else
        {///binlog�����ݸ�ʽ���󣬲���kv
            char szBuf[64];
            CwxCommon::snprintf(pTss->m_szBuf2K,
                2047,
                "Can't unpack binlog, sid=%s",
                CwxCommon::toString(m_cursor->getHeader().getSid(), szBuf));
        }
        uiSkipNum ++;
        if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 2;
        iRet = m_binLog->next(m_cursor);
        if (0 == iRet) return 0; ///<����β��
        if (-1 == iRet)
        {///<ʧ��
            strcpy(pTss->m_szBuf2K, m_cursor->getErrMsg());
            err_num = CWX_MQ_INNER_ERR;
            bClose = true;
            return -1;
        }
    }while(1);
    return 0;
}

CwxMqQueueMgr::CwxMqQueueMgr()
{
}

CwxMqQueueMgr::~CwxMqQueueMgr()
{
    map<string, CwxMqQueue*>::iterator iter =  m_nameQueues.begin();
    while(iter != m_nameQueues.end())
    {
        delete iter->second;
        iter++;
    }
    m_nameQueues.clear();
    m_idQueues.clear();
}

int CwxMqQueueMgr::init(CwxBinLogMgr* binLog,
                        map<string, CWX_UINT64> const& queueSid,
                        map<string, CwxMqConfigQueue> const& queueInfo)
{
    CWX_UINT32 uiId = 1;
    CwxMqQueue* mq = NULL;
    map<string, CWX_UINT64>::const_iterator iter=queueSid.begin();
    map<string, CwxMqConfigQueue>::const_iterator iter_info ;
    string errMsg;
    while(iter != queueSid.end())
    {
        iter_info = queueInfo.find(iter->first);
        CWX_ASSERT(iter_info != queueInfo.end());
        mq = new CwxMqQueue(uiId,
            iter->first,
            iter_info->second.m_strUser,
            iter_info->second.m_strPasswd,
            iter->second,
            binLog);
        if (!CwxMqPoco::parseSubsribe(iter_info->second.m_strSubScribe, mq->getSubscribe(), errMsg))
        {
            delete mq;
            return -1;
        }
        m_nameQueues[iter->first] = mq;
        m_idQueues[uiId++] = mq;
        iter++;
    }
    return 0;
}
