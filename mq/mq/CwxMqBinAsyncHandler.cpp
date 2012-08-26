#include "CwxMqBinAsyncHandler.h"
#include "CwxMqApp.h"

map<CWX_UINT64, CwxMqBinAsyncHandlerSession* > CwxMqBinAsyncHandler::m_sessionMap;  ///<session��map��keyΪsession id
list<CwxMqBinAsyncHandlerSession*>  CwxMqBinAsyncHandler::m_freeSession; ///<��Ҫ�رյ�session


///���һ��������
void CwxMqBinAsyncHandlerSession::addConn(CwxMqBinAsyncHandler* conn){
    CWX_ASSERT(m_conns.find(conn->getConnId()) == m_conns.end());
    m_conns[conn->getConnId()] = conn;
}

///���캯��
CwxMqBinAsyncHandler::CwxMqBinAsyncHandler(CwxMqApp* pApp,
                                           CwxAppChannel* channel,
                                           CWX_UINT32 uiConnId):CwxAppHandler4Channel(channel)
{
    m_bReport = false;
    m_uiConnId = uiConnId;
    m_ullSentSeq = 0;
    m_syncSession = NULL;
    m_pApp = pApp;
    m_uiRecvHeadLen = 0;
    m_uiRecvDataLen = 0;
    m_recvMsgData = 0;
    m_ullSessionId = 0;
    m_tss = NULL;
}

///��������
CwxMqBinAsyncHandler::~CwxMqBinAsyncHandler(){
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    m_recvMsgData = NULL;
}

///�ͷ���Դ
void CwxMqBinAsyncHandler::destroy(CwxMqApp* app){
    map<CWX_UINT64, CwxMqBinAsyncHandlerSession* >::iterator iter = m_sessionMap.begin();
    while(iter != m_sessionMap.end()){
        if (iter->second->m_pCursor) app->getBinLogMgr()->destoryCurser(iter->second->m_pCursor);
        delete iter->second;
        iter++;
    }
    m_sessionMap.clear();
}

void CwxMqBinAsyncHandler::doEvent(CwxMqApp* app, CwxMqTss* tss, CwxMsgBlock*& msg){
    if (CwxEventInfo::CONN_CREATED == msg->event().getEvent()){///���ӽ���
        CwxAppChannel* channel =app->getAsyncDispChannel();
        if (channel->isRegIoHandle(msg->event().getIoHandle())){
            CWX_ERROR(("Handler[%] is register, it's a big bug. exit....", msg->event().getIoHandle()));
            app->stop();
            return;
        }
        CwxMqBinAsyncHandler* pHandler = new CwxMqBinAsyncHandler(app, channel, app->reactor()->getNextConnId());
        ///��ȡ���ӵ���Դ��Ϣ
        CwxINetAddr  remoteAddr;
        CwxSockStream stream(msg->event().getIoHandle());
        stream.getRemoteAddr(remoteAddr);
        pHandler->m_unPeerPort = remoteAddr.getPort();
        if (remoteAddr.getHostIp(tss->m_szBuf2K, 2047)){
            pHandler->m_strPeerHost = tss->m_szBuf2K;
        }
        ///����handle��io��open handler
        pHandler->setHandle(msg->event().getIoHandle());
        if (0 != pHandler->open()){
            CWX_ERROR(("Failure to register sync handler[%d], from:%s:%u", pHandler->getHandle(), pHandler->m_strPeerHost.c_str(), pHandler->m_unPeerPort));
            delete pHandler;
            return;
        }
        ///���ö����tss����
        pHandler->m_tss = (CwxMqTss*)CwxTss::instance();
        CWX_INFO(("Accept sync connection from %s:%u",  pHandler->m_strPeerHost.c_str(), pHandler->m_unPeerPort));
    }else{
        CWX_ERROR(("Unkwown event type:%d", msg->event().getEvent()));
    }
}

///�ͷŹرյ�session
void CwxMqBinAsyncHandler::dealClosedSession(CwxMqApp* app, CwxMqTss* ){
    list<CwxMqBinAsyncHandlerSession*>::iterator iter;
    CwxMqBinAsyncHandler* handler;
    ///��ȡ�û�object����
    if (m_freeSession.begin() != m_freeSession.end()){
        iter = m_freeSession.begin();
        while(iter != m_freeSession.end()){
            ///session������closed״̬
            CWX_ASSERT((*iter)->m_bClosed);
            CWX_INFO(("Close sync session from host:%s", (*iter)->m_strHost.c_str()));
            ///��session��session��map��ɾ��
            m_sessionMap.erase((*iter)->m_ullSessionId);
            ///��ʼ�ر�����
            map<CWX_UINT32, CwxMqBinAsyncHandler*>::iterator conn_iter = (*iter)->m_conns.begin();
            while(conn_iter != (*iter)->m_conns.end()){
                handler = conn_iter->second;
                (*iter)->m_conns.erase(handler->getConnId());
                handler->close();///��Ϊͬ������
                conn_iter = (*iter)->m_conns.begin();
            }
            ///�ͷŶ�Ӧ��cursor
            if ((*iter)->m_pCursor){
                app->getBinLogMgr()->destoryCurser((*iter)->m_pCursor);
            }
            delete *iter;
            iter++;
        }
        m_freeSession.clear();
    }
}


/**
@brief ���ӿɶ��¼�������-1��close()�ᱻ����
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinAsyncHandler::onInput(){
    ///������Ϣ
    int ret = CwxAppHandler4Channel::recvPackage(getHandle(),
        m_uiRecvHeadLen,
        m_uiRecvDataLen,
        m_szHeadBuf,
        m_header,
        m_recvMsgData);
    ///���û�н�����ϣ�0����ʧ�ܣ�-1�����򷵻�
    if (1 != ret) return ret;
    ///���յ�һ�����������ݰ�����Ϣ����
    ret = recvMessage();
    ///���û���ͷŽ��յ����ݰ����ͷ�
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    this->m_recvMsgData = NULL;
    this->m_uiRecvHeadLen = 0;
    this->m_uiRecvDataLen = 0;
    return ret;
}

//1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
int CwxMqBinAsyncHandler::onConnClosed(){
    CWX_INFO(("CwxMqBinAsyncHandler: conn closed, conn_id=%u", m_uiConnId));
    ///һ�����ӹرգ�������sessionʧЧ
    if (m_syncSession){
        ///������Ӷ�Ӧ��session����
        if (m_sessionMap.find(m_ullSessionId) != m_sessionMap.end()){
            CWX_INFO(("CwxMqBinAsyncHandler: conn closed, conn_id=%u", m_uiConnId));
            if (!m_syncSession->m_bClosed){
                ///��session���Ϊclose
                m_syncSession->m_bClosed = true;
                ///��session�ŵ���Ҫ�Ƿ��session�б�
                m_freeSession.push_back(m_syncSession);
            }
            ///�����Ӵ�session��������ɾ������˴����ӽ���delete
            m_syncSession->m_conns.erase(m_uiConnId);
        }
    }
    return -1;
}

///�յ���Ϣ
int CwxMqBinAsyncHandler::recvMessage(){
    if (CwxMqPoco::MSG_TYPE_SYNC_REPORT == m_header.getMsgType()){
        return recvSyncReport(m_tss);
    }else if (CwxMqPoco::MSG_TYPE_SYNC_SESSION_REPORT == m_header.getMsgType()){
        return recvSyncNewConnection(m_tss);
    }else if (CwxMqPoco::MSG_TYPE_SYNC_DATA_REPLY == m_header.getMsgType()){
        return recvSyncReply(m_tss);
    }else if (CwxMqPoco::MSG_TYPE_SYNC_DATA_CHUNK_REPLY == m_header.getMsgType()){
        return recvSyncChunkReply(m_tss);
    }
    ///ֱ�ӹر�����
    CWX_ERROR(("Recv invalid msg type:%u from host:%s:%u, close connection.", m_header.getMsgType(), m_strPeerHost.c_str(), m_unPeerPort));
    return -1;
}

int CwxMqBinAsyncHandler::recvSyncReport(CwxMqTss* pTss){
    int iRet = 0;
    CWX_UINT64 ullSid = 0;
    bool bNewly = false;
    CWX_UINT32 uiChunk = 0;
    char const* subscribe = NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    char const* sign=NULL;
    bool bzip = false;
    CwxMsgBlock* msg = NULL;
    CWX_INFO(("Recv report from host:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
    do{
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_ERROR(("Report package is empty, from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        ///��ֹ�ظ�report sid����cursor���ڣ���ʾ�Ѿ������һ��
        if (m_syncSession){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Can't report sync sid duplicate.");
            CWX_ERROR(("Report is duplicate, from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
        iRet = CwxMqPoco::parseReportData(pTss->m_pReader,
            m_recvMsgData,
            ullSid,
            bNewly,
            uiChunk,
            subscribe,
            user,
            passwd,
            sign,
            bzip,
            pTss->m_szBuf2K);
        if (CWX_MQ_ERR_SUCCESS != iRet){
            CWX_ERROR(("Failure to parse report msg, err=%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        CWX_INFO(("Recv report from:%s:%u, sid=%s, from_new=%s, chunk=%u, subscribe=%s, user=%s, passwd=%s, sign=%s, zip=%s",
            m_strPeerHost.c_str(),
            m_unPeerPort,
            CwxCommon::toString(ullSid, pTss->m_szBuf2K, 10),
            bNewly?"yes":"no",
            uiChunk,
            subscribe?subscribe:"",
            user?user:"",
            passwd?passwd:"",
            sign?sign:"",
            bzip?"yes":"no"));

        if (m_pApp->getConfig().getCommon().m_bMaster){
            if (m_pApp->getConfig().getMaster().m_async.getUser().length()){
                if ( (m_pApp->getConfig().getMaster().m_async.getUser() != user) ||
                    (m_pApp->getConfig().getMaster().m_async.getPasswd() != passwd))
                {
                    iRet = CWX_MQ_ERR_FAIL_AUTH;
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                    CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
                    break;
                }
            }
        }else{
            if (m_pApp->getConfig().getSlave().m_async.getUser().length()){
                if ( (m_pApp->getConfig().getSlave().m_async.getUser() != user) ||
                    (m_pApp->getConfig().getSlave().m_async.getPasswd() != passwd))
                {
                    iRet = CWX_MQ_ERR_FAIL_AUTH;
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                    CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
                    break;
                }
            }
        }
        string strSubcribe=subscribe?subscribe:"";
        string strErrMsg;
        m_syncSession = new CwxMqBinAsyncHandlerSession();
        if (!CwxMqPoco::parseSubsribe(strSubcribe, m_syncSession->m_subscribe, strErrMsg)){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Invalid subscribe[%s], err=%s",
                subscribe,
                strErrMsg.c_str());
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            delete m_syncSession;
            m_syncSession = NULL;
            break;
        }
        m_syncSession->m_strHost = m_strPeerHost;
        m_syncSession->m_uiChunk = uiChunk;
        m_syncSession->m_bZip = bzip;
        m_syncSession->m_strSign = sign;
        if ((m_syncSession->m_strSign != CWX_MQ_CRC32) &&
            (m_syncSession->m_strSign != CWX_MQ_MD5))
        {//���ǩ������CRC32��MD5�������
            m_syncSession->m_strSign.erase();
        }
        if (m_syncSession->m_uiChunk){
            if (m_syncSession->m_uiChunk > CwxMqConfigCmn::MAX_CHUNK_SIZE_KB) m_syncSession->m_uiChunk = CwxMqConfigCmn::MAX_CHUNK_SIZE_KB;
            if (m_syncSession->m_uiChunk < CwxMqConfigCmn::MIN_CHUNK_SIZE_KB) m_syncSession->m_uiChunk = CwxMqConfigCmn::MIN_CHUNK_SIZE_KB;
            m_syncSession->m_uiChunk *= 1024;
        }
        if (bNewly){///��sidΪ�գ���ȡ��ǰ���sid-1
            ullSid = m_pApp->getBinLogMgr()->getMaxSid();
            if (ullSid) ullSid--;
        }
        m_syncSession->reformSessionId();
        ///��session���뵽session��map
        while(m_sessionMap.find(m_syncSession->m_ullSessionId) != m_sessionMap.end()){
            m_syncSession->reformSessionId();
        }
        m_sessionMap[m_syncSession->m_ullSessionId]=m_syncSession;
        m_ullSessionId = m_syncSession->m_ullSessionId;
        m_syncSession->addConn(this);
        ///�ظ�iRet��ֵ
        iRet = CWX_MQ_ERR_SUCCESS;
        ///����binlog��ȡ��cursor
        CwxBinLogCursor* pCursor = m_pApp->getBinLogMgr()->createCurser(ullSid);
        if (!pCursor){
            iRet = CWX_MQ_ERR_ERROR;
            strcpy(pTss->m_szBuf2K, "Failure to create cursor");
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        if (!bNewly){
            if (ullSid && ullSid < m_pApp->getBinLogMgr()->getMinSid()){
                m_pApp->getBinLogMgr()->destoryCurser(pCursor);
                iRet = CWX_MQ_ERR_LOST_SYNC;
                char szBuf1[64], szBuf2[64];
                sprintf(pTss->m_szBuf2K, "Lost sync state, report sid:%s, min sid:%s",
                    CwxCommon::toString(ullSid, szBuf1),
                    CwxCommon::toString(m_pApp->getBinLogMgr()->getMinSid(), szBuf2));
                CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
                break;
            }
        }
        ///����cursor
        m_syncSession->m_pCursor = pCursor;
        m_syncSession->m_ullStartSid = ullSid;

        ///����session id����Ϣ
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packReportDataReply(pTss->m_pWriter,
            msg,
            m_header.getTaskId(),
            m_syncSession->m_ullSessionId,
            pTss->m_szBuf2K))
        {
            CWX_ERROR(("Failure to pack sync data reply, err=%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            return -1;
        }
        msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
        if (!putMsg(msg)){
            CwxMsgBlockAlloc::free(msg);
            CWX_ERROR(("Failure push msg to send queue. from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            return -1;
        }
        ///������һ��binlog
        int iState = syncSendBinLog(pTss);
        if (-1 == iState){
            iRet = CWX_MQ_ERR_ERROR;
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }else if (0 == iState){///����continue����Ϣ
            channel()->regRedoHander(this);
        }
        return 0;
    }while(0);
    ///����һ������
    CWX_ASSERT(CWX_MQ_ERR_SUCCESS != iRet);
    CwxMsgBlock* pBlock = NULL;
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncErr(pTss->m_pWriter,
        pBlock,
        m_header.getTaskId(),
        iRet,
        pTss->m_szBuf2K,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to create binlog reply package, err:%s", pTss->m_szBuf2K));
        return -1;
    }
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (!putMsg(msg)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure push msg to send queue. from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
        return -1;
    }
    return 0;
}

int CwxMqBinAsyncHandler::recvSyncNewConnection(CwxMqTss* pTss){
    int iRet = 0;
    CWX_UINT64 ullSession = 0;
    CwxMsgBlock* msg = NULL;
    do{
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_ERROR(("Session connect-report package is empyt, from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        ///��ֹ�ظ�report sid����cursor���ڣ���ʾ�Ѿ������һ��
        if (m_syncSession){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Can't report sync sid duplicatly.");
            CWX_ERROR(("Session connect-report is duplicate, from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        ///��ȡ�����session id
        iRet = CwxMqPoco::parseReportNewConn(pTss->m_pReader,
            m_recvMsgData,
            ullSession,
            pTss->m_szBuf2K);
        if (CWX_MQ_ERR_SUCCESS != iRet){
            CWX_ERROR(("Failure to parse report new conn msg, err=%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        if (m_sessionMap.find(ullSession) == m_sessionMap.end()){
            iRet = CWX_MQ_ERR_ERROR;
            char szTmp[64];
            CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Session[%s] doesn't exist", CwxCommon::toString(ullSession, szTmp, 10));
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        m_syncSession = m_sessionMap.find(ullSession)->second;
        m_ullSessionId = m_syncSession->m_ullSessionId;
        m_syncSession->addConn(this);
        ///������һ��binlog
        int iState = syncSendBinLog(pTss);
        if (-1 == iState){
            iRet = CWX_MQ_ERR_ERROR;
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }else if (0 == iState){///����continue����Ϣ
            channel()->regRedoHander(this);
        }
        return 0;
    }while(0);
    ///����һ������
    CWX_ASSERT(CWX_MQ_ERR_SUCCESS != iRet);
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncErr(pTss->m_pWriter,
        msg,
        m_header.getTaskId(),
        iRet,
        pTss->m_szBuf2K,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to pack sync data reply, err=%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
        return -1;
    }
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (!putMsg(msg)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure push msg to send queue. from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
        return -1;
    }
    return 0;
}

int CwxMqBinAsyncHandler::recvSyncReply(CwxMqTss* pTss){
    int iRet = CWX_MQ_ERR_SUCCESS;
    CWX_UINT64 ullSeq = 0;
    CwxMsgBlock* msg = NULL;
    do {
        if (!m_syncSession){ ///������Ӳ���ͬ��״̬�����Ǵ���
            strcpy(pTss->m_szBuf2K, "Client no in sync state");
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_ERROR(("Sync reply package is empty, from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
        iRet = CwxMqPoco::parseSyncDataReply(pTss->m_pReader,
            m_recvMsgData,
            ullSeq,
            pTss->m_szBuf2K);
        if (CWX_MQ_ERR_SUCCESS != iRet){
            CWX_ERROR(("Failure to parse sync_data reply package, err:%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        if (ullSeq != m_ullSentSeq){
            char szTmp1[64];
            char szTmp2[64];
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Seq[%s] is not same with the connection's[%s].",
                CwxCommon::toString(ullSeq, szTmp1, 10),
                CwxCommon::toString(m_ullSentSeq, szTmp2, 10));
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            break;
        }
        ///������һ��binlog
        int iState = syncSendBinLog(pTss);
        if (-1 == iState){
            CWX_ERROR(("%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            return -1; ///�ر�����
        }else if (0 == iState){///����continue����Ϣ
            channel()->regRedoHander(this);
        }
        return 0;
    } while(0);
    ///����һ������
    CWX_ASSERT(CWX_MQ_ERR_SUCCESS != iRet);
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncErr(pTss->m_pWriter,
        msg,
        m_header.getTaskId(),
        iRet,
        pTss->m_szBuf2K,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to pack sync data reply, err=%s, from:%s:%u", pTss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
        return -1;
    }
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (!putMsg(msg)){
        CwxMsgBlockAlloc::free(msg);
        CWX_ERROR(("Failure push msg to send queue. from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
        return -1;
    }
    return 0;
}

int CwxMqBinAsyncHandler::recvSyncChunkReply(CwxMqTss* pTss){
    return recvSyncReply(pTss);
}


/**
@brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinAsyncHandler::onRedo(){
    ///�ж��Ƿ��пɷ��͵���Ϣ
    if (m_syncSession->m_ullSid < m_pApp->getBinLogMgr()->getMaxSid()){
        ///������һ��binlog
        int iState = syncSendBinLog(m_tss);
        if (-1 == iState){
            CWX_ERROR(("%s, from:%s:%u", m_tss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
            CwxMsgBlock* msg=NULL;
            if (CWX_MQ_ERR_ERROR != CwxMqPoco::packSyncErr(m_tss->m_pWriter,
                msg,
                m_header.getTaskId(),
                CWX_MQ_ERR_ERROR,
                m_tss->m_szBuf2K,
                m_tss->m_szBuf2K))
            {
                CWX_ERROR(("Failure to pack sync data reply, err=%s, from:%s:%u", m_tss->m_szBuf2K, m_strPeerHost.c_str(), m_unPeerPort));
                return -1;
            }
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
            if (!putMsg(msg)){
                CwxMsgBlockAlloc::free(msg);
                CWX_ERROR(("Failure push msg to send queue. from:%s:%u", m_strPeerHost.c_str(), m_unPeerPort));
                return -1;
            }
        }else if (0 == iState){///����continue����Ϣ
            channel()->regRedoHander(this);
        }
    }else{
        ///����ע��
        channel()->regRedoHander(this);
    }
    ///����
    return 0;
}


///0��δ����һ��binlog��
///1��������һ��binlog��
///-1��ʧ�ܣ�
int CwxMqBinAsyncHandler::syncSendBinLog(CwxMqTss* pTss){
    int iRet = 0;
    CwxMsgBlock* pBlock = NULL;
    CWX_UINT32 uiSkipNum = 0;
    CWX_UINT32 uiKeyLen = 0;
    CWX_UINT32 uiTotalLen = 0;
    CWX_UINT64 ullSeq = m_syncSession->m_ullSeq;
    if (m_syncSession->m_pCursor->isUnseek()){//��binlog�Ķ�ȡcursor���գ���λ
        if (1 != (iRet = syncSeekToReportSid(pTss))) return iRet;
    }

    if (m_syncSession->m_uiChunk)  pTss->m_pWriter->beginPack();
    while(1){
        if ( 1 != (iRet = syncSeekToBinlog(pTss, uiSkipNum))) break;
        //�����Ƶ���һ����¼λ��
        m_syncSession->m_bNext = true;
        if (!m_syncSession->m_uiChunk){
            iRet = syncPackOneBinLog(pTss->m_pWriter,
                pBlock,
                ullSeq,
                pTss->m_pBinlogData,
                pTss->m_szBuf2K);
            break;
        }else{
            iRet = syncPackMultiBinLog(pTss->m_pWriter,
                pTss->m_pItemWriter,
                pTss->m_pBinlogData,
                uiKeyLen,
                pTss->m_szBuf2K);
            if (1 == iRet){
                uiTotalLen += uiKeyLen;
                if (uiTotalLen >= m_syncSession->m_uiChunk) break;
            }
            if (-1 == iRet) break;
            continue;
        }
    }

    if (-1 == iRet) return -1;

    if (!m_syncSession->m_uiChunk){ ///������chunk
        if (0 == iRet) return 0; ///û������
    }else{
        if (0 == uiTotalLen) return 0;
        //add sign
        if (m_syncSession->m_strSign.length()){
            if (m_syncSession->m_strSign == CWX_MQ_CRC32){//CRC32ǩ��
                CWX_UINT32 uiCrc32 = CwxCrc32::value(pTss->m_pWriter->getMsg(), pTss->m_pWriter->getMsgSize());
                if (!pTss->m_pWriter->addKeyValue(CWX_MQ_CRC32, (char*)&uiCrc32, sizeof(uiCrc32))){
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Failure to add key value, err:%s", pTss->m_pWriter->getErrMsg());
                    CWX_ERROR((pTss->m_szBuf2K));
                    return -1;
                }
            } else if (m_syncSession->m_strSign == CWX_MQ_MD5){//md5ǩ��
                CwxMd5 md5;
                unsigned char szMd5[16];
                md5.update((unsigned char*)pTss->m_pWriter->getMsg(), pTss->m_pWriter->getMsgSize());
                md5.final(szMd5);
                if (!pTss->m_pWriter->addKeyValue(CWX_MQ_MD5, (char*)szMd5, 16)){
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Failure to add key value, err:%s", pTss->m_pWriter->getErrMsg());
                    CWX_ERROR((pTss->m_szBuf2K));
                    return -1;
                }
            }
        }
        pTss->m_pWriter->pack();
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packMultiSyncData(0,
            pTss->m_pWriter->getMsg(),
            pTss->m_pWriter->getMsgSize(),
            pBlock,
            ullSeq,
            m_syncSession->m_bZip,
            pTss->m_szBuf2K))
        {
            return -1;
        }
    }
    ///����svr���ͣ��������ݰ�
    pBlock->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_ASYNC);
    pBlock->send_ctrl().setHostId(0);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    if (!putMsg(pBlock)){
        CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Failure to send binlog");
        CWX_ERROR((pTss->m_szBuf2K));
        CwxMsgBlockAlloc::free(pBlock);
        return -1;
    }
    m_ullSentSeq = ullSeq;
    m_syncSession->m_ullSeq++;
    return 1; ///������һ����Ϣ
}

//1���ɹ���0��̫��-1������
int CwxMqBinAsyncHandler::syncSeekToReportSid(CwxMqTss* tss){
    int iRet = 0;
    if (m_syncSession->m_pCursor->isUnseek()){//��binlog�Ķ�ȡcursor���գ���λ
        if (m_syncSession->m_ullStartSid < m_pApp->getBinLogMgr()->getMaxSid()){
            iRet = m_pApp->getBinLogMgr()->seek(m_syncSession->m_pCursor, m_syncSession->m_ullStartSid);
            if (-1 == iRet){
                CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to seek,  err:%s", m_syncSession->m_pCursor->getErrMsg());
                CWX_ERROR((tss->m_szBuf2K));
                return -1;
            }else if (0 == iRet){
                char szBuf1[64];
                char szBuf2[64];
                CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Should seek to sid[%s] with max_sid[[%s], but not.",
                    CwxCommon::toString(m_syncSession->m_ullStartSid, szBuf1),
                    CwxCommon::toString(m_pApp->getBinLogMgr()->getMaxSid(), szBuf2));
                CWX_ERROR((tss->m_szBuf2K));
                return 0;
            }
            ///���ɹ���λ�����ȡ��ǰ��¼
            m_syncSession->m_bNext = m_syncSession->m_ullStartSid == m_syncSession->m_pCursor->getHeader().getSid()?true:false;
        }else{///����Ҫͬ�����͵�sid��С�ڵ�ǰ��С��sid��������Ϊ����״̬
            return 0;///���״̬
        }
    }
    return 1;
}


///-1��ʧ�ܣ�1���ɹ�
int CwxMqBinAsyncHandler::syncPackOneBinLog(CwxPackageWriter* writer,
                                            CwxMsgBlock*& block,
                                            CWX_UINT64 ullSeq,
                                            CwxKeyValueItem const* pData,
                                            char* szErr2K)
{
    ///�γ�binlog���͵����ݰ�
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncData(writer,
        block,
        0,
        m_syncSession->m_pCursor->getHeader().getSid(),
        m_syncSession->m_pCursor->getHeader().getDatetime(),
        *pData,
        m_syncSession->m_pCursor->getHeader().getGroup(),
        m_syncSession->m_strSign.c_str(),
        m_syncSession->m_bZip,
        ullSeq,
        szErr2K))
    {
        ///�γ����ݰ�ʧ��
        CWX_ERROR(("Failure to pack binlog package, err:%s", szErr2K));
        return -1;
    }
    return 1;
}

///-1��ʧ�ܣ����򷵻�������ݵĳߴ�
int CwxMqBinAsyncHandler::syncPackMultiBinLog(CwxPackageWriter* writer,
                                              CwxPackageWriter* writer_item,
                                              CwxKeyValueItem const* pData,
                                              CWX_UINT32&  uiLen,
                                              char* szErr2K)
{
    ///�γ�binlog���͵����ݰ�
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncDataItem(writer_item,
        m_syncSession->m_pCursor->getHeader().getSid(),
        m_syncSession->m_pCursor->getHeader().getDatetime(),
        *pData,
        m_syncSession->m_pCursor->getHeader().getGroup(),
        NULL,
        szErr2K))
    {
        ///�γ����ݰ�ʧ��
        CWX_ERROR(("Failure to pack binlog package, err:%s", szErr2K));
        return -1;
    }
    if (!writer->addKeyValue(CWX_MQ_M, writer_item->getMsg(), writer_item->getMsgSize(),true)){
        ///�γ����ݰ�ʧ��
        CwxCommon::snprintf(szErr2K, 2047, "Failure to pack binlog package, err:%s", writer->getErrMsg());
        CWX_ERROR((szErr2K));
        return -1;
    }
    uiLen = CwxPackage::getKvLen(strlen(CWX_MQ_M),  writer_item->getMsgSize());
    return 1;
}

//1�����ּ�¼��0��û�з��֣�-1������
int CwxMqBinAsyncHandler::syncSeekToBinlog(CwxMqTss* tss, CWX_UINT32& uiSkipNum){
    int iRet = 0;
    if (m_syncSession->m_bNext){
        iRet = m_pApp->getBinLogMgr()->next(m_syncSession->m_pCursor);
        if (0 == iRet) return 0; ///���״̬
        if (-1 == iRet){///<ʧ��
            CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to seek cursor, err:%s", m_syncSession->m_pCursor->getErrMsg());
            CWX_ERROR((tss->m_szBuf2K));
            return -1;
        }
    }
    bool bFind = false;
    CWX_UINT32 uiDataLen = m_syncSession->m_pCursor->getHeader().getLogLen();
    ///׼��data��ȡ��buf
    char* szData = tss->getBuf(uiDataLen);        
    ///��ȡdata
    iRet = m_pApp->getBinLogMgr()->fetch(m_syncSession->m_pCursor, szData, uiDataLen);
    if (-1 == iRet){//��ȡʧ��
        CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to fetch data, err:%s", m_syncSession->m_pCursor->getErrMsg());
        CWX_ERROR((tss->m_szBuf2K));
        return -1;
    }
    uiSkipNum++;
    m_syncSession->m_bNext = false;
    while(1){
        bFind = false;
        do{
            if (!tss->m_pReader->unpack(szData, uiDataLen, false,true)){
                CWX_ERROR(("Can't unpack binlog, sid=%s", CwxCommon::toString(m_syncSession->m_pCursor->getHeader().getSid(), tss->m_szBuf2K)));
                break;
            }
            ///��ȡCWX_MQ_D��key����Ϊ����data����
            tss->m_pBinlogData = tss->m_pReader->getKey(CWX_MQ_D);
            if (!tss->m_pBinlogData){
                CWX_ERROR(("Can't find key[%s] in binlog, sid=%s", CWX_MQ_D,
                    CwxCommon::toString(m_syncSession->m_pCursor->getHeader().getSid(), tss->m_szBuf2K)));
                break;
            }
            bFind = true;
        }while(0);
        if (bFind){
            if (CwxMqPoco::isSubscribe(m_syncSession->m_subscribe,
                m_syncSession->m_pCursor->getHeader().getGroup()))
            {
                break;
            }
        }
        iRet = m_pApp->getBinLogMgr()->next(m_syncSession->m_pCursor);
        if (0 == iRet){
            m_syncSession->m_bNext = true;
            return 0; ///���״̬
        }
        if (-1 == iRet){///<ʧ��
            CwxCommon::snprintf(tss->m_szBuf2K, 2047, "Failure to seek cursor, err:%s", m_syncSession->m_pCursor->getErrMsg());
            CWX_ERROR((tss->m_szBuf2K));
            return -1;
        }
        uiSkipNum ++;
        if (!CwxMqPoco::isContinueSeek(uiSkipNum)){
            return 0;///δ���״̬
        }
    };
    return 1;
}
