#include "CwxMqBinFetchHandler.h"
#include "CwxMqApp.h"
#include "CwxMsgHead.h"
/**
@brief ���ӿɶ��¼�������-1��close()�ᱻ����
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinFetchHandler::onInput(){
    ///������Ϣ
    int ret = CwxAppHandler4Channel::recvPackage(getHandle(),
        m_uiRecvHeadLen,
        m_uiRecvDataLen,
        m_szHeadBuf,
        m_header,
        m_recvMsgData);
    if (1 != ret) return ret; ///���ʧ�ܻ�����Ϣû�н����꣬���ء�
    ///��ȡfetch �̵߳�tss����
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    ///֪ͨ�յ�һ����Ϣ
    ret = recvMessage(tss);
    ///���m_recvMsgDataû���ͷţ����Ƿ�m_recvMsgData�ȴ�������һ����Ϣ
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    this->m_recvMsgData = NULL;
    this->m_uiRecvHeadLen = 0;
    this->m_uiRecvDataLen = 0;
    return ret;
}
/**
@brief ֪ͨ���ӹرա�
@return 1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
*/
int CwxMqBinFetchHandler::onConnClosed(){
    return -1;
}

/**
@brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinFetchHandler::onRedo(){
    ///��ȡtssʵ��
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    int iRet = sentBinlog(tss);
    if (0 == iRet){///�����ȴ���Ϣ
        m_conn.m_bWaiting = true;
        channel()->regRedoHander(this);
    }else if (-1 == iRet){///���󣬹ر�����
        return -1;
    }
    m_conn.m_bWaiting = false;
    return 0;
}

/**
@brief ֪ͨ�������һ����Ϣ�ķ��͡�<br>
ֻ����Msgָ��FINISH_NOTICE��ʱ��ŵ���.
@param [in,out] msg ���뷢����ϵ���Ϣ��������NULL����msg���ϲ��ͷţ�����ײ��ͷš�
@return 
CwxMsgSendCtrl::UNDO_CONN�����޸����ӵĽ���״̬
CwxMsgSendCtrl::RESUME_CONN�������Ӵ�suspend״̬��Ϊ���ݽ���״̬��
CwxMsgSendCtrl::SUSPEND_CONN�������Ӵ����ݽ���״̬��Ϊsuspend״̬
*/
CWX_UINT32 CwxMqBinFetchHandler::onEndSendMsg(CwxMsgBlock*& msg){
    ///��ȡtssʵ��
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    int iRet = m_pApp->getQueueMgr()->endSendMsg(m_conn.m_strQueueName,
        msg->event().m_ullArg,
        true,
        tss->m_szBuf2K);
    //0���ɹ���-1��ʧ�ܣ�-2�����в�����
    if (-1 == iRet){//�ڲ����󣬴�ʱ����ر�����
        CWX_ERROR(("Queue[%s]: Failure to endSendMsg�� err: %s",
            m_conn.m_strQueueName.c_str(), 
            tss->m_szBuf2K));
    }else if (-2 == iRet){//���в�����
        CWX_ERROR(("Queue[%s]: No queue"));
    }
    //��msg��queue mgr���������㲻���ͷ�
    msg = NULL;
    return CwxMsgSendCtrl::UNDO_CONN;
}

/**
@brief ֪ͨ�����ϣ�һ����Ϣ����ʧ�ܡ�<br>
ֻ����Msgָ��FAIL_NOTICE��ʱ��ŵ���.
@param [in,out] msg ����ʧ�ܵ���Ϣ��������NULL����msg���ϲ��ͷţ�����ײ��ͷš�
@return void��
*/
void CwxMqBinFetchHandler::onFailSendMsg(CwxMsgBlock*& msg){
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    backMq(msg->event().m_ullArg, tss);
    //��msg��queue mgr���������㲻���ͷ�
    msg = NULL;
}

///�յ�һ����Ϣ��0���ɹ���-1��ʧ��
int CwxMqBinFetchHandler::recvMessage(CwxMqTss* pTss){
    if (CwxMqPoco::MSG_TYPE_FETCH_DATA == m_header.getMsgType()){///mq�Ļ�ȡ��Ϣ
        return fetchMq(pTss);
    }else if (CwxMqPoco::MSG_TYPE_CREATE_QUEUE == m_header.getMsgType()){///�������е���Ϣ
        return createQueue(pTss);
    }else if (CwxMqPoco::MSG_TYPE_DEL_QUEUE == m_header.getMsgType()){///ɾ�����е���Ϣ
        return delQueue(pTss);
    }
    ///��������Ϣ���򷵻ش���
    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", m_header.getMsgType());
    CWX_ERROR((pTss->m_szBuf2K));
    CwxMsgBlock* block = packEmptyFetchMsg(pTss, CWX_MQ_ERR_ERROR, pTss->m_szBuf2K);
    if (!block){
        CWX_ERROR(("No memory to malloc package"));
        return -1;
    }
    if (-1 == replyFetchMq(pTss, block, false, true)) return -1;
    return 0;
}

///fetch mq
int CwxMqBinFetchHandler::fetchMq(CwxMqTss* pTss){
    int iRet = CWX_MQ_ERR_SUCCESS;
    bool bBlock = false;
    bool bClose = false;
    char const* queue_name = NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    CwxMsgBlock* block = NULL;
    do{
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        iRet = CwxMqPoco::parseFetchMq(pTss->m_pReader,
            m_recvMsgData,
            bBlock,
            queue_name,
            user,
            passwd,
            pTss->m_szBuf2K);
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break; 
        ///�����ǰmq�Ļ�ȡ����waiting״̬��û�ж��ȷ�ϣ���ֱ�Ӻ���
        if (m_conn.m_bWaiting) return 0;
        ///����ǵ�һ�λ�ȡ���߸ı�����Ϣ���ˣ���У�����Ȩ��
        if (!m_conn.m_strQueueName.length() ||  ///��һ�λ�ȡ
            m_conn.m_strQueueName != queue_name ///�¶���
            )
        {
            m_conn.m_strQueueName.erase(); ///��յ�ǰ����
            string strQueue = queue_name?queue_name:"";
            string strUser = user?user:"";
            string strPasswd = passwd?passwd:"";
            ///��Ȩ���е��û���������
            iRet = m_pApp->getQueueMgr()->authQueue(strQueue, strUser, strPasswd);
            ///������в����ڣ��򷵻ش�����Ϣ
            if (0 == iRet){
                iRet = CWX_MQ_ERR_ERROR;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "No queue:%s", strQueue.c_str());
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            ///���Ȩ�޴����򷵻ش�����Ϣ
            if (-1 == iRet){
                iRet = CWX_MQ_ERR_FAIL_AUTH;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            ///��ʼ������
            m_conn.reset();
            ///���ö�����
            m_conn.m_strQueueName = strQueue;
        }
        ///�����Ƿ�block
        m_conn.m_bBlock = bBlock;
        ///������Ϣ
        int ret = sentBinlog(pTss);
        if (0 == ret){///�ȴ���Ϣ
            m_conn.m_bWaiting = true;
            channel()->regRedoHander(this);
            return 0;
        }else if (-1 == ret){///����ʧ�ܣ��ر����ӡ��������ڴ治�㡢���ӹرյȵ����صĴ���
            return -1;
        }
        //������һ����Ϣ
        m_conn.m_bWaiting = false;
        return 0;
    }while(0);

    ///�ظ�������Ϣ
    m_conn.m_bWaiting = false;
    block = packEmptyFetchMsg(pTss, iRet, pTss->m_szBuf2K);
    if (!block)
    {///����ռ����ֱ�ӹر�����
        CWX_ERROR(("No memory to malloc package"));
        return -1;
    }
    ///�ظ�ʧ�ܣ�ֱ�ӹر�����
    if (-1 == replyFetchMq(pTss, block, false, bClose)) return -1;
    return 0;

}
///create queue
int CwxMqBinFetchHandler::createQueue(CwxMqTss* pTss)
{
    int iRet = CWX_MQ_ERR_SUCCESS;
    char const* queue_name = NULL;
    char const* user = NULL;
    char const* passwd = NULL;
    char const* scribe = NULL;
    char const* auth_user = NULL;
    char const* auth_passwd = NULL;
    CWX_UINT64 ullSid = 0;
    do{
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        iRet = CwxMqPoco::parseCreateQueue(pTss->m_pReader,
            m_recvMsgData,
            queue_name,
            user,
            passwd,
            scribe,
            auth_user,
            auth_passwd,
            ullSid,
            pTss->m_szBuf2K);
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break;
        //У��Ȩ��
        if (m_pApp->getConfig().getMq().m_mq.getUser().length()){
            if ((m_pApp->getConfig().getMq().m_mq.getUser() != auth_user) ||
                (m_pApp->getConfig().getMq().m_mq.getPasswd() != auth_passwd))
            {
                iRet = CWX_MQ_ERR_FAIL_AUTH;
                strcpy(pTss->m_szBuf2K, "No auth");
                break;
            }
        }
        //������
        if (!strlen(queue_name)){
            iRet = CWX_MQ_ERR_ERROR;
            strcpy(pTss->m_szBuf2K, "queue name is empty");
            break;
        }
        if (strlen(queue_name) > CWX_MQ_MAX_QUEUE_NAME_LEN){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue name[%s] is too long, max:%u", queue_name, CWX_MQ_MAX_QUEUE_NAME_LEN);
            break;
        }
        if (!CwxMqQueueMgr::isInvalidQueueName(queue_name)){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue name[%s] is invalid, charactor must be in [a-z, A-Z, 0-9, -, _]", queue_name);
            break;
        }
        if (strlen(user) > CWX_MQ_MAX_QUEUE_USER_LEN){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's user name[%s] is too long, max:%u", user, CWX_MQ_MAX_QUEUE_USER_LEN);
            break;
        }
        if (strlen(passwd) > CWX_MQ_MAX_QUEUE_PASSWD_LEN){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's passwd [%s] is too long, max:%u", passwd, CWX_MQ_MAX_QUEUE_PASSWD_LEN);
            break;
        }
        if (!strlen(scribe)) scribe = "*";
        if (strlen(scribe) > CWX_MQ_MAX_QUEUE_SCRIBE_LEN){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's scribe [%s] is too long, max:%u", scribe, CWX_MQ_MAX_QUEUE_SCRIBE_LEN);
            break;
        }
        string strErrMsg;
        if (!CwxMqPoco::isValidSubscribe(string(scribe), strErrMsg)){
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "%s", strErrMsg.c_str());
            break;
        }
        if (0 == ullSid) ullSid = m_pApp->getBinLogMgr()->getMaxSid();
        iRet = m_pApp->getQueueMgr()->addQueue(string(queue_name),
            ullSid,
            string(user),
            string(passwd),
            string(scribe),
            pTss->m_szBuf2K);
        if (1 == iRet){//�ɹ�
            iRet = CWX_MQ_ERR_SUCCESS;
            break;
        }else if (0 == iRet){//exist
            iRet = CWX_MQ_ERR_ERROR;
            strcpy(pTss->m_szBuf2K, "Queue exists");
            break;
        }else{//�ڲ�����
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
    }while(0);

    CwxMsgBlock* block = NULL;
    iRet = CwxMqPoco::packCreateQueueReply(pTss->m_pWriter, block, iRet, pTss->m_szBuf2K, pTss->m_szBuf2K);
    if (CWX_MQ_ERR_SUCCESS != iRet){
        CWX_ERROR(("Failure to pack create-queue-reply, err:%s", pTss->m_szBuf2K));
        return -1;
    }
    block->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    block->send_ctrl().setHostId(0);
    block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(block)){///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply create queue"));
        CwxMsgBlockAlloc::free(block);
        return -1;
    }
    return 0;
}
///del queue
int CwxMqBinFetchHandler::delQueue(CwxMqTss* pTss){
    int iRet = CWX_MQ_ERR_SUCCESS;
    char const* queue_name = NULL;
    char const* user = NULL;
    char const* passwd = NULL;
    char const* auth_user = NULL;
    char const* auth_passwd = NULL;
    do{
        if (!m_recvMsgData){
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
        iRet = CwxMqPoco::parseDelQueue(pTss->m_pReader,
            m_recvMsgData,
            queue_name,
            user,
            passwd,
            auth_user,
            auth_passwd,
            pTss->m_szBuf2K);
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break;
        //У��Ȩ��
        if (m_pApp->getConfig().getMq().m_mq.getUser().length()){
            if ((m_pApp->getConfig().getMq().m_mq.getUser() != auth_user) ||
                (m_pApp->getConfig().getMq().m_mq.getPasswd() != auth_passwd))
            {
                iRet = CWX_MQ_ERR_FAIL_AUTH;
                strcpy(pTss->m_szBuf2K, "No auth");
                break;
            }
        }
        iRet = m_pApp->getQueueMgr()->authQueue(string(queue_name), string(user), string(passwd));
        if (0 == iRet){//���в�����
            iRet = CWX_MQ_ERR_ERROR;
            CwxCommon::snprintf(pTss->m_szBuf2K,  2047, "Queue[%s] doesn't exists", queue_name);
            break;
        }
        ///���Ȩ�޴����򷵻ش�����Ϣ
        if (-1 == iRet){
            iRet = CWX_MQ_ERR_FAIL_AUTH;
            strcpy(pTss->m_szBuf2K, "No auth");
            break;
        }
        iRet = m_pApp->getQueueMgr()->delQueue(string(queue_name), pTss->m_szBuf2K);
        if (1 == iRet){//�ɹ�
            iRet = CWX_MQ_ERR_SUCCESS;
            break;
        }else if (0 == iRet){//exist
            iRet = CWX_MQ_ERR_ERROR;
            strcpy(pTss->m_szBuf2K, "Queue exists");
            break;
        }else{//�ڲ�����
            iRet = CWX_MQ_ERR_ERROR;
            break;
        }
    }while(0);

    CwxMsgBlock* block = NULL;
    iRet = CwxMqPoco::packDelQueueReply(pTss->m_pWriter, block, iRet, pTss->m_szBuf2K, pTss->m_szBuf2K);
    if (CWX_MQ_ERR_SUCCESS != iRet){
        CWX_ERROR(("Failure to pack del-queue-reply, err:%s", pTss->m_szBuf2K));
        return -1;
    }
    block->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    block->send_ctrl().setHostId(0);
    block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(block)){///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply delete queue"));
        CwxMsgBlockAlloc::free(block);
        return -1;
    }
    return 0;
}


CwxMsgBlock* CwxMqBinFetchHandler::packEmptyFetchMsg(CwxMqTss* pTss,
                                                     int iRet,
                                                     char const* szErrMsg
                                                     )
{
    CwxMsgBlock* pBlock = NULL;
    CwxKeyValueItem kv;
    iRet = CwxMqPoco::packFetchMqReply(pTss->m_pWriter,
        pBlock,
        iRet,
        szErrMsg,
        0,
        0,
        kv,
        0,
        pTss->m_szBuf2K);
    return pBlock;
}


int CwxMqBinFetchHandler::replyFetchMq(CwxMqTss* pTss,
                                       CwxMsgBlock* msg,
                                       bool bBinlog,
                                       bool bClose)
{
    msg->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    msg->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    msg->send_ctrl().setHostId(0);
    if (bBinlog){///�����binlog����ʱ��Ҫ������ϵ�ʱ��֪ͨ
        if (bClose) ///�Ƿ�ر�����
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FAIL_NOTICE | CwxMsgSendCtrl::FINISH_NOTICE|CwxMsgSendCtrl::CLOSE_NOTICE);
        else 
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FAIL_NOTICE | CwxMsgSendCtrl::FINISH_NOTICE);
    }else{
        if (bClose)
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
        else
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    }
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(msg)){///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply fetch mq"));
        ///�����binlog����Ҫ����Ϣ���ص�mq manager
        if (bBinlog) 
            backMq(msg->event().m_ullArg, pTss);
        else///�����ͷ���Ϣ
            CwxMsgBlockAlloc::free(msg);
        return -1;
    }
    return 0;
}

void CwxMqBinFetchHandler::backMq(CWX_UINT64 ullSid, CwxMqTss* pTss){
    int iRet = m_pApp->getQueueMgr()->endSendMsg(m_conn.m_strQueueName,
        ullSid,
        false,
        pTss->m_szBuf2K);
    if (0 != iRet){
        char szSid[32];
        if(-1 == iRet){///���д���
            CWX_ERROR(("Failure to back queue[%s]'s message, err:%s",
                m_conn.m_strQueueName.c_str(),
                pTss->m_szBuf2K));
        }else{///���в�����
            CWX_ERROR(("Failure to back queue[%s]'s message for no existing",
                m_conn.m_strQueueName.c_str()));
        }
    }
}

///������Ϣ��0��û����Ϣ���ͣ�1������һ����-1������ʧ��
int CwxMqBinFetchHandler::sentBinlog(CwxMqTss* pTss){
    CwxMsgBlock* pBlock=NULL;
    int err_no = CWX_MQ_ERR_SUCCESS;
    int iState = 0;
    iState = m_pApp->getQueueMgr()->getNextBinlog(pTss,
        m_conn.m_strQueueName,
        pBlock,
        err_no,
        pTss->m_szBuf2K);
    if (-1 == iState){///��ȡ��Ϣʧ��
        CWX_ERROR(("Failure to read binlog ,err:%s", pTss->m_szBuf2K));
        pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_ERROR, pTss->m_szBuf2K);
        if (!pBlock){
            CWX_ERROR(("No memory to malloc package"));
            return -1;
        }
        ///��log����binlog��
        if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
        return 1;
    }else if (0 == iState){ ///�Ѿ����
        if (!m_conn.m_bBlock){///�������block���ͣ��ظ�û����Ϣ
            pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_ERROR, "No message");
            if (!pBlock){
                CWX_ERROR(("No memory to malloc package"));
                return -1;
            }
            if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
            return 1;
        }
        //δ��ɣ��ȴ�block
        return 0;
    }else if (1 == iState){///��ȡ��һ����Ϣ
        if (0 != replyFetchMq(pTss, pBlock, true, false)){
            return -1;
        }
        return 1;
    }else if (2 == iState){//δ���
        return 0;
    }
    //��ʱ��iStateΪ-2
    //no queue
    CWX_ERROR(("Not find queue:%s", m_conn.m_strQueueName.c_str()));
    string strErr = string("No queue:") + m_conn.m_strQueueName;
    pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_ERROR, strErr.c_str());
    if (!pBlock){
        CWX_ERROR(("No memory to malloc package"));
        return -1;
    }
    if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
    return 1;
}
