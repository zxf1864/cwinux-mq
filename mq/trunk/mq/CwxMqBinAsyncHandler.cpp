#include "CwxMqBinAsyncHandler.h"
#include "CwxMqApp.h"
///���캯��
CwxMqBinAsyncHandler::CwxMqBinAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
{
    m_pApp = pApp;
    m_pCursor = NULL;
    m_uiRecvHeadLen = 0;
    m_uiRecvDataLen = 0;
    m_recvMsgData = 0;
}
///��������
CwxMqBinAsyncHandler::~CwxMqBinAsyncHandler()
{
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    if (m_dispatch->m_pCursor) m_pApp->getBinLogMgr()->destoryCurser(m_dispatch->m_pCursor);
}

/**
@brief ���ӿɶ��¼�������-1��close()�ᱻ����
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinAsyncHandler::onInput()
{
    int ret = CwxAppHandler4Channel::recvPackage(getHandle(),
        m_uiRecvHeadLen,
        m_uiRecvDataLen,
        m_szHeadBuf,
        m_header,
        m_recvMsgData);
    if (1 != ret) return ret;
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    ret = recvMessage(tss);
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    this->m_recvMsgData = NULL;
    this->m_uiRecvHeadLen = 0;
    this->m_uiRecvDataLen = 0;
    return ret;

}

//1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
int CwxMqBinAsyncHandler::onConnClosed()
{
    return -1;
}



///���ӹرպ���Ҫ������
int CwxMqBinAsyncHandler::recvMessage(CwxMqTss* pTss)
{
    CWX_UINT64 ullSid = 0;
    CWX_UINT32 uiChunk = 0;
    bool  bNewly = false;
    char const* subscribe=NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    int iRet = CWX_MQ_SUCCESS;
    int iState = 0;
    do 
    {
        if (CwxMqPoco::MSG_TYPE_SYNC_DATA_REPLY == m_header.getMsgType())
        {
            if (!m_dispatch.m_bSync)
            { ///������Ӳ���ͬ��״̬�����Ǵ���
                strcpy(pTss->m_szBuf2K, "Client no in sync state");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_INVALID_MSG_TYPE;
                break;
            }
            ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
            iRet = CwxMqPoco::parseSyncDataReply(pTss->m_pReader,
                m_recvMsgData,
                ullSid,
                pTss->m_szBuf2K);
            if (CWX_MQ_SUCCESS != iRet)
            {
                CWX_DEBUG(("Failure to parse sync_data reply package, err:%s", pTss->m_szBuf2K));
                break;
            }
            ///��鷵�ص�sid
            if (ullSid != m_dispatch.m_ullSid)
            {
                char szBuf1[64];
                char szBuf2[64];
                CwxCommon::snprintf(pTss->m_szBuf2K, "Reply sid[%s] is not the right sid[%s], close conn.",
                    CwxCommon::toString(ullSid, szBuf1, 10),
                    CwxCommon::toString(m_dispatch.m_ullSid, szBuf2, 10));
                CWX_ERROR((pTss->m_szBuf2K));
               iRet = CWX_MQ_INVALID_SID;
                break;
            }
            ///������һ��binlog
            iState = sendBinLog(m_pApp, m_dispatch, pTss);
            if (-1 == iState)
            {
                CWX_ERROR((pTss->m_szBuf2K));
                return -1; ///�ر�����
            }
            else if (0 == iState)
            {///����continue����Ϣ
                m_pApp->getAsyncDispChannel()->regRedoHander(this);
                m_dispatch.m_bContinue = true;
            }
            ///����
            return 0;
        }
        else if (CwxMqPoco::MSG_TYPE_SYNC_REPORT == m_header.getMsgType())
        {
            if (m_dispatch->m_pCursor)
            {
                iRet = CWX_MQ_INVALID_MSG;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Can't report sync sid duplicatly.");
                CWX_ERROR((pTss->m_szBuf2K));
                break;
            }
            ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
            iRet = CwxMqPoco::parseReportData(pTss,
                msg,
                ullSid,
                bNewly,
                uiChunk,
                subscribe,
                user,
                passwd,
                pTss->m_szBuf2K);
            if (CWX_MQ_SUCCESS != iRet)
            {///�������ڣ�����󷵻�
                CWX_ERROR(("Failure to parse report msg, err=%s%s", pTss->m_szBuf2K));
                break;
            }
            if (m_pApp->getConfig().getCommon().m_bMaster)
            {
                if (m_pApp->getConfig().getMaster().m_async_bin.getUser().length())
                {
                    if ( (m_pApp->getConfig().getMaster().m_async_bin.getUser() != user) ||
                        (m_pApp->getConfig().getMaster().m_async_bin.getPasswd() != passwd))
                    {
                        iRet = CWX_MQ_FAIL_AUTH;
                        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                        CWX_DEBUG((pTss->m_szBuf2K));
                        break;
                    }
                }
            }
            else
            {
                if (m_pApp->getConfig().getSlave().m_async_bin.getUser().length())
                {
                    if ( (m_pApp->getConfig().getSlave().m_async_bin.getUser() != user) ||
                        (m_pApp->getConfig().getSlave().m_async_bin.getPasswd() != passwd))
                    {
                        iRet = CWX_MQ_FAIL_AUTH;
                        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                        CWX_DEBUG((pTss->m_szBuf2K));
                        break;
                    }
                }
            }
            string strSubcribe=subscribe?subscribe:"";
            string strErrMsg;
            if (!CwxMqPoco::parseSubsribe(subscribe, m_dispatch->m_subscribe, strErrMsg))
            {
                iRet = CWX_MQ_INVALID_SUBSCRIBE;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Invalid subscribe[%s], err=%s", strSubcribe.c_str(), strErrMsg.c_str());
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            m_dispatch->m_bSync = true;
            m_dispatch->m_uiChunk = uiChunk;
            if (m_dispatch.m_uiChunk)
            {
                if (m_dispatch.m_uiChunk > CwxMqConfigCmn::MAX_CHUNK_SIZE_KB) m_dispatch.m_uiChunk = CwxMqConfigCmn::MAX_CHUNK_SIZE_KB;
                if (m_dispatch.m_uiChunk < CwxMqConfigCmn::MIN_CHUNK_SIZE_KB) m_dispatch.m_uiChunk = CwxMqConfigCmn::MIN_CHUNK_SIZE_KB;
                m_dispatch.m_uiChunk *= 1024;
            }
            if (bNewly)
            {///��sidΪ�գ���ȡ��ǰ���sid-1
                ullSid = m_pApp->getBinLogMgr()->getMaxSid();
                if (ullSid) ullSid--;
            }
            ///�ظ�iRet��ֵ
            iRet = CWX_MQ_SUCCESS;
            ///����binlog��ȡ��cursor
            CwxBinLogCursor* pCursor = m_pApp->getBinLogMgr()->createCurser();
            if (!pCursor)
            {
                iRet = CWX_MQ_INNER_ERR;
                strcpy(pTss->m_szBuf2K, "Failure to create cursor");
                CWX_ERROR((pTss->m_szBuf2K));
                break;
            }
            ///����cursor
            m_dispatch->m_pCursor = pCursor;
            m_dispatch->m_ullStartSid = ullSid;
            m_dispatch->m_bSync = true;
            if (-1 == iState)
            {
                CWX_ERROR((pTss->m_szBuf2K));
                return -1; ///�ر�����
            }
            else if (0 == iState)
            {///����continue����Ϣ
                m_pApp->getAsyncDispChannel()->regRedoHander(this);
                m_dispatch.m_bContinue = true;
            }
            ///����
            return 0;
        }
        else
        {
            ///��������Ϣ���򷵻ش���
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", msg->event().getMsgHeader().getMsgType());
            iRet = CWX_MQ_INVALID_MSG_TYPE;
            CWX_ERROR((pTss->m_szBuf2K));
        }

    } while(0);
    

    ///�γ�ʧ��ʱ��Ļظ����ݰ�
    CwxMsgBlock* pBlock = NULL;
    if (CWX_MQ_SUCCESS != CwxMqPoco::packReportDataReply(pTss,
        pBlock,
        m_header.getTaskId(),
        iRet,
        ullSid,
        pTss->m_szBuf2K,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to create binlog reply package, err:%s", pTss->m_szBuf2K));
        return -1;
    }
    ///���ͻظ������ݰ�
    pBlock->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_ASYNC_BIN);
    pBlock->send_ctrl().setHostId(0);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (0 != this->putMsg(pBlock))
    {
        CWX_ERROR(("Failure to send msg to reciever, conn[%u]", msg->event().getConnId()));
        CwxMsgBlockAlloc::free(pBlock);
        return -1;
        ///�ر�����
    }
    return 0;
}

///����binlog������ϵ���Ϣ
int CwxMqBinAsyncHandler::onEndSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    map<CWX_UINT32, CwxMqDispatchConn*>::iterator iter = m_dispatchConns->m_clientMap.find(msg->event().getConnId());
    ///���ӱ������
    CWX_ASSERT(iter != m_dispatchConns->m_clientMap.end());
    CwxMqDispatchConn* conn = iter->second;
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    CWX_UINT64 ullSid = msg->event().m_ullArg;
    if (!conn->m_window.isSyncing()){ ///������Ӳ���ͬ��״̬�����Ǵ���
        strcpy(pTss->m_szBuf2K, "Client no in sync state");
        CWX_ERROR((pTss->m_szBuf2K));
        //�ر�����
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return 1;
    }
    if(!conn->m_window.recvOneMsg(ullSid))
    {
        char buf[64];
        CWX_ERROR(("Async msg can't find in window, sid[%s", CwxCommon::toString(ullSid, buf, 10)));
        CWX_ASSERT(0);
    }
    conn->m_recvWindow.insert(ullSid);
    ///������һ��binlog
    int iState = sendBinLog(m_pApp, conn, pTss);
    if (-1 == iState)
    {
        CWX_ERROR(("Failure to send bin log, err:%s", pTss->m_szBuf2K));
    }
    else if (0 == iState)
    {///����continue����Ϣ
        noticeContinue(pTss, conn);
    }
    ///����
    return 1;
}


///��������Ϣ��ʱ��
int CwxMqBinAsyncHandler::onUserEvent(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    CwxMqDispatchConn* conn = NULL;
    map<CWX_UINT32, CwxMqDispatchConn*>::iterator iter = m_dispatchConns->m_clientMap.find(msg->event().getConnId());
    if (iter != m_dispatchConns->m_clientMap.end())
    {
        conn = iter->second;
        conn->m_bContinue = false;
        int iState = sendBinLog(m_pApp, conn, pTss);
        if (-1 == iState)
        {
            CWX_ERROR(("Failure to send bin log, , err:%s", pTss->m_szBuf2K));
        }
        else if (0 == iState)
        {///����continue����Ϣ
            noticeContinue(pTss, conn);
        }
    }

    return 1;
}


void CwxMqBinAsyncHandler::dispatch(CwxMqTss* pTss)
{
    int iState = 0;
    CwxMqDispatchConn* conn=(CwxMqDispatchConn *)m_dispatchConns->m_connTail.head();
    //���ȷַ���ɵ���Ϣ
    while(conn)
    {
        iState = sendBinLog(m_pApp, conn, pTss);
        if (-1 == iState)
        {
            CWX_ERROR(("Failure to dispatch message, conn[%u]", conn->m_window.getConnId()));
            //�ر�����
            m_pApp->noticeCloseConn(conn->m_window.getConnId());
        }
        else if (0 == iState) //δ���
        {///����continue����Ϣ
            noticeContinue(pTss, conn);
        }
        conn = conn->m_next;
    }
}

///0��δ���״̬��
///1�����״̬��
///-1��ʧ�ܣ�
int CwxMqBinAsyncHandler::sendBinLog(CwxMqApp* pApp,
                                  CwxMqDispatchConn* conn,
                                  CwxMqTss* pTss)
{
    int iRet = 0;
    CWX_UINT32 uiDataLen;
    char* pBuf = NULL;
    CWX_ASSERT(conn->m_window.isSyncing());
    CWX_ASSERT(conn->m_window.getHandle());
    //if (conn->m_window.getUsedSize()) return 1; ///<������Ϣ�ڷ��Ͷ���
    if (!conn->m_window.isEnableSend()) return 1; ///<�����ʹ����������򲻷��ͣ�Ϊ���״̬
    if (conn->m_recvWindow.size() > pApp->getConfig().getCommon().m_uiDispatchWindowSize) return 1;
    CwxBinLogCursor* pCursor = (CwxBinLogCursor*)conn->m_window.getHandle();
    CwxMsgBlock* pBlock = NULL;
    CwxKeyValueItem const* pItem = NULL;
    if (pApp->getBinLogMgr()->isUnseek(pCursor))
    {//��binlog�Ķ�ȡcursor���գ���λ
        if (conn->m_window.getStartSid() < pApp->getBinLogMgr()->getMaxSid())
        {
            iRet = pApp->getBinLogMgr()->seek(pCursor, conn->m_window.getStartSid());
            if (-1 == iRet)
            {
                CWX_ERROR(("Failure to seek,  err:%s", pCursor->getErrMsg()));
                return -1;
            }
            else if (0 == iRet)
            {
                char szBuf1[64];
                char szBuf2[64];
                CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Should seek to sid[%s] with max_sid[[%s], but not.",
                    CwxCommon::toString(conn->m_window.getStartSid(), szBuf1),
                    CwxCommon::toString(pApp->getBinLogMgr()->getMaxSid(), szBuf2));
                CWX_ERROR((pTss->m_szBuf2K));
                return -1;
            }
            ///���ɹ���λ�����ȡ��ǰ��¼
            conn->m_bNext = conn->m_window.getStartSid()==pCursor->getHeader().getSid()?true:false;
        }
        else
        {///����Ҫͬ�����͵�sid��С�ڵ�ǰ��С��sid��������Ϊ����״̬
            return 1;///���״̬
        }
    }
    ///����Ƿ�����һ��binlog�����ƶ�cursor
    if (conn->m_bNext)
    {
        iRet = pApp->getBinLogMgr()->next(pCursor);
        if (0 == iRet)
        {
            return 1; ///���״̬
        }
        if (-1 == iRet)
        {///<ʧ��
            CWX_ERROR(("Failure to seek cursor, err:%s", pCursor->getErrMsg()));
            return -1;
        }
    }
    conn->m_bNext = true;
    CWX_UINT32 uiSkipNum = 0;
    while(1)
    {
        while (!CwxMqPoco::isSubscribe(conn->m_subscribe,
            conn->m_bSync,
            pCursor->getHeader().getGroup(),
            pCursor->getHeader().getType()))
        {
            iRet = pApp->getBinLogMgr()->next(pCursor);
            if (0 == iRet) return 1; ///���״̬
            if (-1 == iRet)
            {///<ʧ��
                CWX_ERROR(("Failure to seek cursor, err:%s", pCursor->getErrMsg()));
                return -1;
            }
            uiSkipNum ++;
            if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 0; ///δ���״̬
            continue;
        }
        ///��ȡbinlog��data����
        uiDataLen = pCursor->getHeader().getLogLen();
        ///׼��data��ȡ��buf
        pBuf = pTss->getBuf(uiDataLen);        
        ///��ȡdata
        iRet = pApp->getBinLogMgr()->fetch(pCursor, pBuf, uiDataLen);
        if (-1 == iRet)
        {//��ȡʧ��
            CWX_ERROR(("Failure to fetch data, err:%s", pCursor->getErrMsg()));
            return -1;
        }
        ///unpack data�����ݰ�
        if (pTss->m_pReader->unpack(pBuf, uiDataLen, false, true))
        {
            ///��ȡCWX_MQ_DATA��key����Ϊ����data����
            pItem = pTss->m_pReader->getKey(CWX_MQ_DATA);
            if (pItem)
            {
                ///�γ�binlog���͵����ݰ�
                if (CWX_MQ_SUCCESS != CwxMqPoco::packSyncData(pTss,
                    pBlock,
                    0,
                    pCursor->getHeader().getSid(),
                    pCursor->getHeader().getDatetime(),
                    *pItem,
                    pCursor->getHeader().getGroup(),
                    pCursor->getHeader().getType(),
                    pCursor->getHeader().getAttr(),
                    pTss->m_szBuf2K))
                {
                    ///�γ����ݰ�ʧ��
                    CWX_ERROR(("Failure to pack binlog package, err:%s", pTss->m_szBuf2K));
                }
                else
                {
                    ///����svr���ͣ��������ݰ�
                    pBlock->send_ctrl().setConnId(conn->m_window.getConnId());
                    pBlock->send_ctrl().setSvrId(conn->m_window.getSvrId());
                    pBlock->send_ctrl().setHostId(0);
                    pBlock->event().m_ullArg = pCursor->getHeader().getSid();
                    pBlock->event().setTaskId(pCursor->getHeader().getSid()&0xFFFFFFFF);
                    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FINISH_NOTICE);
                    if (0 != pApp->sendMsgByConn(pBlock))
                    {
                        CWX_ERROR(("Failure to send binlog"));
                        CwxMsgBlockAlloc::free(pBlock);
                        return -1;
                    }
                    ///֪ͨ���ڷ�����һ��binlog
                    conn->m_window.sendOneMsg(pCursor->getHeader().getSid());
                    return 1; ///������һ����Ϣ
                }
            }
            else
            {///��ȡ��������Ч                
                CWX_ERROR(("Can't find key[%s] in binlog, sid=%s", CWX_MQ_DATA,
                    CwxCommon::toString(pCursor->getHeader().getSid(), pTss->m_szBuf2K)));
            }            
        }
        else
        {///binlog�����ݸ�ʽ���󣬲���kv
            CWX_ERROR(("Can't unpack binlog, sid=%s", CwxCommon::toString(pCursor->getHeader().getSid(), pTss->m_szBuf2K)));
        }
        uiSkipNum ++;
        if (!CwxMqPoco::isContinueSeek(uiSkipNum)) return 0;
/*        if (conn->m_window.isEnableSend())
        {///�����ʹ���û����,��׼��������һ��
            iRet = pApp->getBinLogMgr()->next(pCursor);
            if (0 == iRet) return 1; ///���״̬
            if (-1 == iRet)
            {///<ʧ��
                CWX_ERROR(("Failure to seek cursor, err:%s", pCursor->getErrMsg()));
                return -1;
            }
        }
        else
        {///<���������������˳�����
            return 0; ///δ���״̬
        }*/
    }
    ///��ԶҲ���ᵽ��
    return 0; ///δ���״̬
}

void CwxMqBinAsyncHandler::noticeContinue(CwxMqTss* , CwxMqDispatchConn* conn)
{
    if (!conn->m_bContinue)
    {
        conn->m_bContinue = true;
        CwxMsgBlock* msg = CwxMsgBlockAlloc::malloc(0);
        msg->event().setSvrId(CwxMqApp::SVR_TYPE_ASYNC);
        msg->event().setEvent(CwxMqApp::MQ_CONTINUE_SEND_EVENT);
        msg->event().setConnId(conn->m_window.getConnId());
        m_pApp->getWriteThreadPool()->append(msg);
    }
}
