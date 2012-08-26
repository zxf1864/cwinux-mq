#include "CwxMqBinRecvHandler.h"
#include "CwxMqApp.h"
#include "CwxZlib.h"

///���ӽ�������Ҫά�����������ݵķַ�
int CwxMqBinRecvHandler::onConnCreated(CwxMsgBlock*& msg, CwxTss* ){
    ///���ӱ�����벻����
    CWX_ASSERT(m_clientMap.find(msg->event().getConnId()) == m_clientMap.end());
    m_clientMap[msg->event().getConnId()] = false;
    CWX_DEBUG(("Add recv conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///���ӹرպ���Ҫ������
int CwxMqBinRecvHandler::onConnClosed(CwxMsgBlock*& msg, CwxTss* ){
    map<CWX_UINT32, bool>::iterator iter = m_clientMap.find(msg->event().getConnId());
    ///���ӱ������
    CWX_ASSERT(iter != m_clientMap.end());
    m_clientMap.erase(iter);
    CWX_DEBUG(("remove recv conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///echo����Ĵ�����
int CwxMqBinRecvHandler::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv){
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    int iRet = CWX_MQ_ERR_SUCCESS;
    char const* user=NULL;
    char const* passwd=NULL;
    map<CWX_UINT32, bool>::iterator conn_iter = m_clientMap.find(msg->event().getConnId());
    CWX_ASSERT(conn_iter != m_clientMap.end());
    bool bAuth = conn_iter->second;
    CWX_UINT64 ullSid = 0;
    do{
        ///binlog���ݽ�����Ϣ
        if (CwxMqPoco::MSG_TYPE_RECV_DATA == msg->event().getMsgHeader().getMsgType())
        {
            CWX_UINT32 uiGroup;
            CwxKeyValueItem const* pData;
            if (m_pApp->getBinLogMgr()->isInvalid()){
                ///���binlog mgr��Ч����ֹͣ����
                iRet = CWX_MQ_ERR_ERROR;
                strcpy(pTss->m_szBuf2K, m_pApp->getBinLogMgr()->getInvalidMsg());
                break;
            }
            if (!msg){
                strcpy(pTss->m_szBuf2K, "No data.");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_ERROR;
                break;
            }

            unsigned long ulUnzipLen = 0;
            bool bZip = msg->event().getMsgHeader().isAttr(CwxMsgHead::ATTR_COMPRESS);
            //�ж��Ƿ�ѹ������
            if (bZip){//ѹ�����ݣ���Ҫ��ѹ
                //����׼����ѹ��buf
                if (!prepareUnzipBuf()){
                    iRet = CWX_MQ_ERR_ERROR;
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Failure to prepare unzip buf, size:%u", m_uiBufLen);
                    CWX_ERROR((pTss->m_szBuf2K));
                    break;
                }
                ulUnzipLen = m_uiBufLen;
                //��ѹ
                if (!CwxZlib::unzip(m_unzipBuf, ulUnzipLen, (const unsigned char*)msg->rd_ptr(), msg->length())){
                    iRet = CWX_MQ_ERR_ERROR;
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Failure to unzip recv msg, msg size:%u, buf size:%u", msg->length(), m_uiBufLen);
                    CWX_ERROR((pTss->m_szBuf2K));
                    break;
                }
            }

            if (CWX_MQ_ERR_SUCCESS != (iRet=CwxMqPoco::parseRecvData(pTss->m_pReader,
                bZip?(char const*)m_unzipBuf:msg->rd_ptr(),
                bZip?ulUnzipLen:msg->length(),
                pData,
                uiGroup,
                user,
                passwd,
                pTss->m_szBuf2K)))
            {
                //�������Ч���ݣ�����
                CWX_DEBUG(("Failure to parse the recieve msg, err=%s", pTss->m_szBuf2K));
                break;
            }
            if (!bAuth && m_pApp->getConfig().getMaster().m_recv.getUser().length()){
                if ((m_pApp->getConfig().getMaster().m_recv.getUser() != user) ||
                    (m_pApp->getConfig().getMaster().m_recv.getPasswd() != passwd))
                {
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                    CWX_DEBUG((pTss->m_szBuf2K));
                    iRet = CWX_MQ_ERR_FAIL_AUTH;
                    break;
                }
                conn_iter->second = true;
            }
            pTss->m_pWriter->beginPack();
            pTss->m_pWriter->addKeyValue(CWX_MQ_D, pData->m_szData, pData->m_uiDataLen, pData->m_bKeyValue);
            pTss->m_pWriter->pack();
            ullSid = m_pApp->nextSid();
            if (0 != m_pApp->getBinLogMgr()->append(ullSid,
                time(NULL),
                uiGroup,
                pTss->m_pWriter->getMsg(),
                pTss->m_pWriter->getMsgSize(),
                pTss->m_szBuf2K))
            {
                CWX_ERROR((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_ERROR;
                break;
            }
            ///����δ�ύ��binlog����
            m_pApp->incUnCommitLogNum();
            ///auto commit
            if (m_pApp->isFirstBinLog() ||
                (m_pApp->getUnCommitLogNum() >= m_pApp->getConfig().getBinLog().m_uiFlushNum)||
                (time(NULL) > (time_t)(m_pApp->getLastCommitTime() + m_pApp->getConfig().getBinLog().m_uiFlushSecond)))
            {
                ///���ﵽ�ύ���������һ���ύ�����ύ
                if (0 != commit(pTss->m_szBuf2K)){
                    CWX_ERROR((pTss->m_szBuf2K));
                    iRet = CWX_MQ_ERR_ERROR;
                    break;
                }
            }
        }else if(CwxMqPoco::MSG_TYPE_RECV_COMMIT == msg->event().getMsgHeader().getMsgType()){
            if (!msg){
                user = "";
                passwd = "";
            }else{
                if (CWX_MQ_ERR_SUCCESS != (iRet=CwxMqPoco::parseCommit(pTss->m_pReader,
                    msg,
                    user,
                    passwd,
                    pTss->m_szBuf2K)))
                {
                    //�������Ч���ݣ�����
                    CWX_DEBUG(("Failure to parse the commit msg, err=%s", pTss->m_szBuf2K));
                    //iRet = CWX_MQ_ERR_ERROR;
                    break;
                }
            }
            if (m_pApp->getConfig().getMaster().m_recv.getUser().length()){
                if ((m_pApp->getConfig().getMaster().m_recv.getUser() != user) ||
                    (m_pApp->getConfig().getMaster().m_recv.getPasswd() != passwd))
                {
                    CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                    CWX_DEBUG((pTss->m_szBuf2K));
                    iRet = CWX_MQ_ERR_FAIL_AUTH;
                    break;
                }
            }

            if (0 != commit(pTss->m_szBuf2K)){
                CWX_ERROR(("Failure to commit the binlog, err=%s", pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_ERROR;
                break;
            }
        }else{
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", msg->event().getMsgHeader().getMsgType());
            CWX_ERROR((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
    }while(0);
    CwxMsgBlock* pBlock = NULL;
    if (CwxMqPoco::MSG_TYPE_RECV_COMMIT==msg->event().getMsgHeader().getMsgType()){
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packCommitReply(pTss->m_pWriter,
            pBlock,
            msg->event().getMsgHeader().getTaskId(),
            iRet,
            pTss->m_szBuf2K,
            pTss->m_szBuf2K))
        {
            CWX_ERROR(("Failure to pack commit reply msg, err=%s", pTss->m_szBuf2K));
            m_pApp->noticeCloseConn(msg->event().getConnId());
            return 1;
        }
    }else{
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packRecvDataReply(pTss->m_pWriter,
            pBlock,
            msg->event().getMsgHeader().getTaskId(),
            iRet,
            ullSid,
            pTss->m_szBuf2K,
            pTss->m_szBuf2K))
        {
            CWX_ERROR(("Failure to pack mq reply msg, err=%s", pTss->m_szBuf2K));
            m_pApp->noticeCloseConn(msg->event().getConnId());
            return 1;
        }
    }
    pBlock->send_ctrl().setConnId(conn_iter->first);
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_RECV);
    pBlock->send_ctrl().setHostId(0);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    if (0 != m_pApp->sendMsgByConn(pBlock)){
        CWX_ERROR(("Failure to reply error msg"));
        CwxMsgBlockAlloc::free(pBlock);
        m_pApp->noticeCloseConn(msg->event().getConnId());
    }
    return 1;
}

///����ͬ��dispatch����Ҫ���ͬ���ĳ�ʱ
int CwxMqBinRecvHandler::onTimeoutCheck(CwxMsgBlock*& , CwxTss* pThrEnv){
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    ///flush binlog
    if (m_pApp->getUnCommitLogNum()){
        if ((m_pApp->getUnCommitLogNum()>=m_pApp->getConfig().getBinLog().m_uiFlushNum) ||
            (time(NULL) > (time_t)(m_pApp->getLastCommitTime() + m_pApp->getConfig().getBinLog().m_uiFlushSecond)))
        {
            if (0 != this->commit(pTss->m_szBuf2K)){
                CWX_ERROR(("Failure to flush binlog ,err=%s", pTss->m_szBuf2K));
            }
        }
    }
    return 1;
}


int CwxMqBinRecvHandler::commit(char* szErr2K){
    int iRet = 0;
    CWX_INFO(("Begin flush bin log......."));
    if (m_pApp->getUnCommitLogNum()){
        m_pApp->clearFirstBinLog();
        iRet = m_pApp->getBinLogMgr()->commit(false, szErr2K);
        m_pApp->zeroUnCommitLogNum();
        m_pApp->setLastCommitTime(time(NULL));
    }
    CWX_INFO(("End flush bin log......."));
    return iRet;
}

bool CwxMqBinRecvHandler::prepareUnzipBuf(){
    if (!m_unzipBuf){
        m_uiBufLen = CWX_MQ_MAX_MSG_SIZE + 4096;
        if (m_uiBufLen < 1024 * 1024) m_uiBufLen = 1024 * 1024;
        m_unzipBuf = new unsigned char[m_uiBufLen];
    }
    return m_unzipBuf!=NULL;
}
