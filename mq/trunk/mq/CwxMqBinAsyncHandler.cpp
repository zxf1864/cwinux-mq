#include "CwxMqBinAsyncHandler.h"
#include "CwxMqApp.h"
///���캯��
CwxMqBinAsyncHandler::CwxMqBinAsyncHandler(CwxMqApp* pApp):m_pApp(pApp)
{
    m_dispatchConns = new CwxMqDispatchConnSet(pApp->getBinLogMgr());
}
///��������
CwxMqBinAsyncHandler::~CwxMqBinAsyncHandler()
{
    if (m_dispatchConns) delete m_dispatchConns;
}

///���ӽ�������Ҫά�����������ݵķַ�
int CwxMqBinAsyncHandler::onConnCreated(CwxMsgBlock*& msg, CwxTss* )
{
    ///���ӱ�����벻����
    CWX_ASSERT(m_dispatchConns->m_clientMap.find(msg->event().getConnId()) == m_dispatchConns->m_clientMap.end());
    CwxMqDispatchConn* conn = new CwxMqDispatchConn(msg->event().getSvrId(),
        msg->event().getHostId(),
        msg->event().getConnId(),
        m_pApp->getConfig().getCommon().m_uiDispatchWindowSize);
    conn->m_bContinue = false;
    ///���÷���װ�ڵ�״̬
    conn->m_window.setState(CwxAppAioWindow::STATE_CONNECTED);
    ///��������ӵ�map��
    m_dispatchConns->m_clientMap[msg->event().getConnId()] = conn;
    CWX_DEBUG(("Add dispatch conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///���ӹرպ���Ҫ������
int CwxMqBinAsyncHandler::onConnClosed(CwxMsgBlock*& msg, CwxTss* )
{
    map<CWX_UINT32, CwxMqDispatchConn*>::iterator iter = m_dispatchConns->m_clientMap.find(msg->event().getConnId());
    ///���ӱ������
    CWX_ASSERT(iter != m_dispatchConns->m_clientMap.end());
    CwxMqDispatchConn* conn = iter->second;
    ///�����Ӵ�map��ɾ��
    m_dispatchConns->m_clientMap.erase(iter);
    ///ɾ��binlog��ȡ��cursor
    if (conn->m_window.getHandle())
    {
        CwxBinLogCursor* pCursor = (CwxBinLogCursor*)conn->m_window.getHandle();
        m_pApp->getBinLogMgr()->destoryCurser(pCursor);
        m_dispatchConns->m_connTail.remove(conn);
    }
    delete conn;
    CWX_DEBUG(("remove dispatch conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///�������Էַ��Ļظ���Ϣ��ͬ��״̬������Ϣ
int CwxMqBinAsyncHandler::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    map<CWX_UINT32, CwxMqDispatchConn*>::iterator iter = m_dispatchConns->m_clientMap.find(msg->event().getConnId());
    ///���ӱ������
    CWX_ASSERT(iter != m_dispatchConns->m_clientMap.end());
    CwxMqDispatchConn* conn = iter->second;
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    CWX_UINT64 ullSid = 0;
    CWX_UINT32 uiChunk = 0;
    bool  bNewly = false;
    char const* subscribe=NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    int iRet = CWX_MQ_SUCCESS;
    int iState = 0;
    if (CwxMqPoco::MSG_TYPE_SYNC_DATA_REPLY == msg->event().getMsgHeader().getMsgType())
    {
        if (!conn->m_window.isSyncing())
        { ///������Ӳ���ͬ��״̬�����Ǵ���
            strcpy(pTss->m_szBuf2K, "Client no in sync state");
            CWX_DEBUG((pTss->m_szBuf2K));
            //�ر�����
            m_pApp->noticeCloseConn(msg->event().getConnId());
            return 1;
        }
        ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
        iRet = CwxMqPoco::parseSyncDataReply(pTss,
            msg,
            ullSid,
            pTss->m_szBuf2K);
        if (CWX_MQ_SUCCESS != iRet)
        {
            CWX_DEBUG(("Failure to parse sync_data reply package, err:%s", pTss->m_szBuf2K));
            //�ر�����
            m_pApp->noticeCloseConn(msg->event().getConnId());
            return 1;
        }
        ///֪ͨ���ʹ����Ѿ��յ���Ϣ
        set<CWX_UINT64>::iterator iter_sid = conn->m_recvWindow.find(ullSid);
        if (iter_sid != conn->m_recvWindow.begin())
        {
            char szBuf1[64];
            char szBuf2[64];
            CWX_UINT64 ullMinSid = 0;
            if (conn->m_recvWindow.size()) ullMinSid = *(conn->m_recvWindow.begin());
            CWX_ERROR(("Dispatch conn[%u]'s reply sid[%s] is not the right sid[%s], close conn.",
                conn->m_window.getConnId(),
                CwxCommon::toString(ullSid, szBuf1, 10),
                CwxCommon::toString(ullMinSid, szBuf2, 10)));
            m_pApp->noticeCloseConn(conn->m_window.getConnId());
            return 1;
        }
        conn->m_recvWindow.erase(iter_sid);
        ///������һ��binlog
        //if (0 == conn->m_window.getUsedSize())
        {
            iState = sendBinLog(m_pApp, conn, pTss);
            if (-1 == iState)
            {
                CWX_ERROR((pTss->m_szBuf2K));
                //�ر�����
                m_pApp->noticeCloseConn(msg->event().getConnId());
            }
            else if (0 == iState)
            {///����continue����Ϣ
                noticeContinue(pTss, conn);
            }
        }
        ///����
        return 1;
    }
    else if (CwxMqPoco::MSG_TYPE_SYNC_REPORT == msg->event().getMsgHeader().getMsgType())
    {
        do
        {
            if (conn->m_window.getHandle())
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
                if (m_pApp->getConfig().getMaster().m_async.getUser().length())
                {
                    if ( (m_pApp->getConfig().getMaster().m_async.getUser() != user) ||
                        (m_pApp->getConfig().getMaster().m_async.getPasswd() != passwd))
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
                if (m_pApp->getConfig().getSlave().m_async.getUser().length())
                {
                    if ( (m_pApp->getConfig().getSlave().m_async.getUser() != user) ||
                        (m_pApp->getConfig().getSlave().m_async.getPasswd() != passwd))
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
            if (!CwxMqPoco::parseSubsribe(subscribe, conn->m_subscribe, strErrMsg))
            {
                iRet = CWX_MQ_INVALID_SUBSCRIBE;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Invalid subscribe[%s], err=%s", strSubcribe.c_str(), strErrMsg.c_str());
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            conn->m_bSync = true;
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
            conn->m_window.setHandle(pCursor);
            conn->m_window.setStartSid(ullSid);
            conn->m_window.setState(CwxAppAioWindow::STATE_SYNCING);
            ///�������ӵ��ַ�����
            m_dispatchConns->m_connTail.push_head(conn);
            iState = sendBinLog(m_pApp, conn, pTss);
            if (-1 == iState)
            {
                iRet = CWX_MQ_INNER_ERR;
                CWX_ERROR((pTss->m_szBuf2K));
                break;
            }
            else if (0 == iState)
            {///����continue����Ϣ
                noticeContinue(pTss, conn);
            }
            return 1; ///����
        }while(0);
    }
    else
    {
        ///��������Ϣ���򷵻ش���
        CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", msg->event().getMsgHeader().getMsgType());
        iRet = CWX_MQ_INVALID_MSG_TYPE;
        CWX_ERROR((pTss->m_szBuf2K));
    }

    ///�γ�ʧ��ʱ��Ļظ����ݰ�
    CwxMsgBlock* pBlock = NULL;
    if (CWX_MQ_SUCCESS != CwxMqPoco::packReportDataReply(pTss,
        pBlock,
        msg->event().getMsgHeader().getTaskId(),
        iRet,
        ullSid,
        pTss->m_szBuf2K,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to create binlog reply package, err:%s", pTss->m_szBuf2K));
        m_pApp->noticeCloseConn(msg->event().getConnId());
        return 1;
    }
    ///���ͻظ������ݰ�
    pBlock->send_ctrl().setConnId(conn->m_window.getConnId());
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_ASYNC);
    pBlock->send_ctrl().setHostId(0);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (0 != m_pApp->sendMsgByConn(pBlock))
    {
        CWX_ERROR(("Failure to send msg to reciever, conn[%u]", msg->event().getConnId()));
        CwxMsgBlockAlloc::free(pBlock);
        ///�ر�����
        m_pApp->noticeCloseConn(msg->event().getConnId());
    }
    return 1;
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
