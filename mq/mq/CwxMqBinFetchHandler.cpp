#include "CwxMqBinFetchHandler.h"
#include "CwxMqApp.h"
#include "CwxMsgHead.h"
/**
@brief ���ӿɶ��¼�������-1��close()�ᱻ����
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinFetchHandler::onInput()
{
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
int CwxMqBinFetchHandler::onConnClosed()
{
    ///�����ǰ���з��͵�mq��Ϣû����ɣ�����մ���Ϣ
    ///���ڷ�commit�Ķ��У���Ϣ������Ϻ�m_ullSendSid����0��
    ///����commit���У���Ϣcommit��m_ullSendSid����0
    if (m_conn.m_ullSendSid)
    {
        CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
        backMq(tss);
        m_conn.m_ullSendSid = 0;
    }
    return -1;
}

/**
@brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinFetchHandler::onRedo()
{
    ///��ȡtssʵ��
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    int iRet = sentBinlog(tss);
    if (0 == iRet)
    {///�����ȴ���Ϣ
        m_conn.m_bWaiting = true;
        channel()->regRedoHander(this);
    }
    else if (-1 == iRet)
    {///���󣬹ر�����
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
CWX_UINT32 CwxMqBinFetchHandler::onEndSendMsg(CwxMsgBlock*& msg)
{
    ///��ȡtssʵ��
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    CWX_ASSERT(!m_conn.m_bSent);
    int iRet = m_pApp->getQueueMgr()->endSendMsg(m_conn.m_strQueueName,
        m_conn.m_ullSendSid,
        true,
        tss->m_szBuf2K);
    //0�������ڣ�1���ɹ���-1��ʧ�ܣ�-2�����в�����
    if (0 == iRet)
    {//��Ϣ�Ѿ���ʱ����һ����commit���͵Ķ���
        char szSid[64];
        CWX_DEBUG(("Queue[%s]: sid[%s] is timeout",
            m_conn.m_strQueueName.c_str(), 
            CwxCommon::toString(m_conn.m_ullSendSid, szSid, 10)));
    }
    else if (-1 == iRet)
    {//�ڲ����󣬴�ʱ����ر�����
        CWX_ERROR(("Queue[%s]: Failure to endSendMsg�� err: %s",
            m_conn.m_strQueueName.c_str(), 
            tss->m_szBuf2K));
    }
    else if (-2 == iRet)
    {//���в�����
        CWX_ERROR(("Queue[%s]: No queue"));
    }
    ///������Ϣ�Ѿ��������
    m_conn.m_bSent = true;
    if (!m_conn.m_bCommit)
    {///�����commit���еģ������m_conn.m_ullSendSid������ȴ�commit����Ϣ
        m_conn.m_ullSendSid = 0;
    }
    else if (1 != iRet)
    {
        ///����commit���в����ڻ��ڲ��������Ϣ���ͳ�ʱ������Ҫ��ա�
        ///�˷��ô����commit����Ϊ��Ϣ�Ѿ������˿��ٷ��͵Ķ���
        m_conn.m_ullSendSid = 0;
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
void CwxMqBinFetchHandler::onFailSendMsg(CwxMsgBlock*& msg)
{
	CWX_ASSERT(m_conn.m_ullSendSid > 0);
	CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
	backMq(tss);
	m_conn.m_ullSendSid = 0;
	//��msg��queue mgr���������㲻���ͷ�
	msg = NULL;
}



///�յ�һ����Ϣ��0���ɹ���-1��ʧ��
int CwxMqBinFetchHandler::recvMessage(CwxMqTss* pTss)
{
    if (CwxMqPoco::MSG_TYPE_FETCH_DATA == m_header.getMsgType())
    {///mq�Ļ�ȡ��Ϣ
        return fetchMq(pTss);
    }
    else if (CwxMqPoco::MSG_TYPE_FETCH_COMMIT == m_header.getMsgType())
    {///mq�ĵ���commit��Ϣ
        return fetchMqCommit(pTss);
    }
    else if (CwxMqPoco::MSG_TYPE_CREATE_QUEUE == m_header.getMsgType())
    {///�������е���Ϣ
        return createQueue(pTss);
    }
    else if (CwxMqPoco::MSG_TYPE_DEL_QUEUE == m_header.getMsgType())
    {///ɾ�����е���Ϣ
        return delQueue(pTss);
    }
    ///��������Ϣ���򷵻ش���
    CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", m_header.getMsgType());
    CWX_ERROR((pTss->m_szBuf2K));
    CwxMsgBlock* block = packEmptyFetchMsg(pTss, CWX_MQ_ERR_INVALID_MSG_TYPE, pTss->m_szBuf2K);
    if (!block)
    {
        CWX_ERROR(("No memory to malloc package"));
        return -1;
    }
    if (-1 == replyFetchMq(pTss, block, false, true)) return -1;
    return 0;
}

///fetch mq
int CwxMqBinFetchHandler::fetchMq(CwxMqTss* pTss)
{
    int iRet = CWX_MQ_ERR_SUCCESS;
    bool bBlock = false;
    bool bClose = false;
    char const* queue_name = NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    CwxMsgBlock* block = NULL;
    CWX_UINT32  timeout = 0;
    do
    {
        if (!m_recvMsgData)
        {
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_NO_MSG;
            break;
        }

        iRet = CwxMqPoco::parseFetchMq(pTss->m_pReader,
            m_recvMsgData,
            bBlock,
            queue_name,
            user,
            passwd,
            timeout,
            pTss->m_szBuf2K);
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break; 
        ///�����ǰmq�Ļ�ȡ����waiting״̬��û�ж��ȷ�ϣ���ֱ�Ӻ���
        if (m_conn.m_bWaiting || !m_conn.m_bSent) 
        {
            return 0;
        }
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
            if (0 == iRet)
            {
                iRet = CWX_MQ_ERR_NO_QUEUE;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "No queue:%s", strQueue.c_str());
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            ///���Ȩ�޴����򷵻ش�����Ϣ
            if (-1 == iRet)
            {
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
        else if (m_conn.m_bCommit && m_conn.m_ullSendSid)
        {//���������commit���͵ģ����ҷ��͵���Ϣû��commit��������commit
            ///��ʱ�ύʧ���ǲ������ظ��ģ�����Ҫ�ظ��������commit����Ϣ
            iRet = m_pApp->getQueueMgr()->commitBinlog(m_conn.m_strQueueName,
                m_conn.m_ullSendSid,
                true,
                0,
                pTss->m_szBuf2K);
            if (0 == iRet)
            {//��Ϣ�����ڣ���ʱ��Ϣ�Ѿ���ʱ
                char szSid[64];
                CWX_DEBUG(("queue[%s]: sid[%s] is timeout", m_conn.m_strQueueName.c_str(), CwxCommon::toString(m_conn.m_ullSendSid, szSid, 10)));
            }
            else if (-1 == iRet)
            {//�ڲ��ύʧ��
                CWX_ERROR(("queue[%s]: Failure to commit msg, err:%s", m_conn.m_strQueueName.c_str(), pTss->m_szBuf2K));
            }
            else if (-2 == iRet)
            {
                CWX_DEBUG(("queue[%s]: No queue", m_conn.m_strQueueName.c_str()));
            }
            //���m_ullSendSid
            m_conn.m_ullSendSid = 0;
        }
        ///�����Ƿ�block
        m_conn.m_bBlock = bBlock;
        ///����timeoutֵ
        m_conn.m_uiTimeout = timeout;
        ///������Ϣ
        int ret = sentBinlog(pTss);
        if (0 == ret)
        {///�ȴ���Ϣ
            m_conn.m_bWaiting = true;
            channel()->regRedoHander(this);
			return 0;
        }
        else if (-1 == ret)
        {///����ʧ�ܣ��ر����ӡ��������ڴ治�㡢���ӹرյȵ����صĴ���
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

///commit mq,����ֵ,0���ɹ���-1��ʧ��
int CwxMqBinFetchHandler::fetchMqCommit(CwxMqTss* pTss)
{
    int iRet = CWX_MQ_ERR_SUCCESS;
    bool bCommit = true;
    CWX_UINT32 uiDelay = 0;
    do
    {
        if (m_recvMsgData)
        {
            iRet = CwxMqPoco::parseFetchMqCommit(pTss->m_pReader,
                m_recvMsgData,
                bCommit,
                uiDelay,
                pTss->m_szBuf2K);

        }
        else
        {
            bCommit = true;
            uiDelay = 0;
        }
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break;
        if (!m_conn.m_bCommit)
        {
            iRet = CWX_MQ_ERR_INVALID_COMMIT;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue is not commit type queue");
            break;
        }
        if (!m_conn.m_bSent)
        {
            iRet = CWX_MQ_ERR_INVALID_COMMIT;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "No message to commit.");
            break;
        }
        if (0 == m_conn.m_ullSendSid)
        {//��ʱ
            iRet = CWX_MQ_ERR_TIMEOUT;
            strcpy(pTss->m_szBuf2K, "Message is timeout.");
            break;
        }


        iRet = m_pApp->getQueueMgr()->commitBinlog(m_conn.m_strQueueName,
            m_conn.m_ullSendSid,
            bCommit,
            uiDelay,
            pTss->m_szBuf2K);

        //���m_ullSendSid
        m_conn.m_ullSendSid = 0;
        if (0 == iRet)
        {//��Ϣ�����ڣ���ʱ��Ϣ�Ѿ���ʱ
            iRet = CWX_MQ_ERR_TIMEOUT;
            strcpy(pTss->m_szBuf2K, "Message is timeout.");
            break;
        }
        else if (-1 == iRet)
        {//�ڲ��ύʧ��
            iRet = CWX_MQ_ERR_INNER_ERR;
            CWX_ERROR(("queue[%s]: Failure to commit msg, err:%s", m_conn.m_strQueueName.c_str(), pTss->m_szBuf2K));
            break;
        }
        else if (-2 == iRet)
        {//���в�����
            iRet = CWX_MQ_ERR_NO_QUEUE;
			CwxCommon::snprintf(pTss->m_szBuf2K, 2047,  "No queue:%s", m_conn.m_strQueueName.c_str());
            break;
        }
        ///�ɹ��ύ��Ϣ
        iRet = CWX_MQ_ERR_SUCCESS;
        pTss->m_szBuf2K[0] = 0x00;
    }while(0);

    CwxMsgBlock* block = NULL;
    iRet = CwxMqPoco::packFetchMqCommitReply(pTss->m_pWriter, block, iRet, pTss->m_szBuf2K, pTss->m_szBuf2K);
    if (CWX_MQ_ERR_SUCCESS != iRet)
    {
        CWX_ERROR(("Failure to pack mq-commit-reply, err:%s", pTss->m_szBuf2K));
        return -1;
    }

    block->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    block->send_ctrl().setHostId(0);
    block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(block))
    {///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply fetch mq commit"));
        CwxMsgBlockAlloc::free(block);
        return -1;
    }
    return 0;

}

///create queue
int CwxMqBinFetchHandler::createQueue(CwxMqTss* pTss)
{
    int iRet = CWX_MQ_ERR_SUCCESS;
    bool bCommit = true;
    char const* queue_name = NULL;
    char const* user = NULL;
    char const* passwd = NULL;
    char const* scribe = NULL;
    char const* auth_user = NULL;
    char const* auth_passwd = NULL;
    CWX_UINT64 ullSid = 0;
    CWX_UINT32 uiDefTimeout = 0;
    CWX_UINT32 uiMaxTimeout = 0;

    do
    {
        if (!m_recvMsgData)
        {
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_NO_MSG;
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
            bCommit,
            uiDefTimeout,
            uiMaxTimeout,
            pTss->m_szBuf2K);
        ///�������ʧ�ܣ�����������Ϣ����
        if (CWX_MQ_ERR_SUCCESS != iRet) break;
        //У��Ȩ��
        if (m_pApp->getConfig().getMq().m_mq.getUser().length())
        {
            if ((m_pApp->getConfig().getMq().m_mq.getUser() != auth_user) ||
                (m_pApp->getConfig().getMq().m_mq.getPasswd() != auth_passwd))
            {
                iRet = CWX_MQ_ERR_FAIL_AUTH;
                strcpy(pTss->m_szBuf2K, "No auth");
                break;
            }
        }
        //������
        if (!strlen(queue_name))
        {
            iRet = CWX_MQ_ERR_NAME_EMPTY;
            strcpy(pTss->m_szBuf2K, "queue name is empty");
            break;
        }
        if (strlen(queue_name) > CWX_MQ_MAX_QUEUE_NAME_LEN)
        {
            iRet = CWX_MQ_ERR_NAME_TOO_LONG;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue name[%s] is too long, max:%u", queue_name, CWX_MQ_MAX_QUEUE_NAME_LEN);
            break;
        }
		if (!CwxMqQueueMgr::isInvalidQueueName(queue_name))
		{
			iRet = CWX_MQ_ERR_INVALID_QUEUE_NAME;
			CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue name[%s] is invalid, charactor must be in [a-z, A-Z, 0-9, -, _]", queue_name);
			break;
		}
        if (strlen(user) > CWX_MQ_MAX_QUEUE_USER_LEN)
        {
            iRet = CWX_MQ_ERR_USER_TO0_LONG;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's user name[%s] is too long, max:%u", user, CWX_MQ_MAX_QUEUE_USER_LEN);
            break;
        }
        if (strlen(passwd) > CWX_MQ_MAX_QUEUE_PASSWD_LEN)
        {
            iRet = CWX_MQ_ERR_PASSWD_TOO_LONG;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's passwd [%s] is too long, max:%u", passwd, CWX_MQ_MAX_QUEUE_PASSWD_LEN);
            break;
        }
        if (!strlen(scribe)) scribe = "*";
        if (strlen(scribe) > CWX_MQ_MAX_QUEUE_SCRIBE_LEN)
        {
            iRet = CWX_MQ_ERR_SCRIBE_TOO_LONG;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Queue's scribe [%s] is too long, max:%u", scribe, CWX_MQ_MAX_QUEUE_SCRIBE_LEN);
            break;
        }
        string strErrMsg;
        if (!CwxMqPoco::isValidSubscribe(string(scribe), strErrMsg))
        {
            iRet = CWX_MQ_ERR_INVALID_SUBSCRIBE;
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "%s", strErrMsg.c_str());
            break;
        }
        //���ó�ʱ��Χ
        if (0 == uiDefTimeout) uiDefTimeout = CWX_MQ_DEF_TIMEOUT_SECOND;
        if (uiDefTimeout < CWX_MQ_MIN_TIMEOUT_SECOND) uiDefTimeout = CWX_MQ_MIN_TIMEOUT_SECOND;
        if (uiDefTimeout > CWX_MQ_MAX_TIMEOUT_SECOND) uiDefTimeout = CWX_MQ_MAX_TIMEOUT_SECOND;
        if (0 == uiMaxTimeout) uiMaxTimeout = CWX_MQ_MAX_TIMEOUT_SECOND;
        if (uiMaxTimeout < CWX_MQ_MIN_TIMEOUT_SECOND) uiMaxTimeout = CWX_MQ_MIN_TIMEOUT_SECOND;
        if (uiMaxTimeout > CWX_MQ_MAX_TIMEOUT_SECOND) uiMaxTimeout = CWX_MQ_MAX_TIMEOUT_SECOND;

		if (uiDefTimeout > uiMaxTimeout) uiDefTimeout = uiMaxTimeout;

        if (0 == ullSid) ullSid = m_pApp->getBinLogMgr()->getMaxSid();

        iRet = m_pApp->getQueueMgr()->addQueue(string(queue_name),
            ullSid,
            bCommit,
            string(user),
            string(passwd),
            string(scribe),
            uiDefTimeout,
            uiMaxTimeout,
            pTss->m_szBuf2K);
        if (1 == iRet)
        {//�ɹ�
            iRet = CWX_MQ_ERR_SUCCESS;
            break;
        }
        else if (0 == iRet)
        {//exist
            iRet = CWX_MQ_ERR_QUEUE_EXIST;
            strcpy(pTss->m_szBuf2K, "Queue exists");
            break;
        }
        else
        {//�ڲ�����
            iRet = CWX_MQ_ERR_INNER_ERR;
            break;
        }
    }while(0);

    CwxMsgBlock* block = NULL;
    iRet = CwxMqPoco::packCreateQueueReply(pTss->m_pWriter, block, iRet, pTss->m_szBuf2K, pTss->m_szBuf2K);
    if (CWX_MQ_ERR_SUCCESS != iRet)
    {
        CWX_ERROR(("Failure to pack create-queue-reply, err:%s", pTss->m_szBuf2K));
        return -1;
    }

    block->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    block->send_ctrl().setHostId(0);
    block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(block))
    {///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply create queue"));
        CwxMsgBlockAlloc::free(block);
        return -1;
    }
    return 0;
}
///del queue
int CwxMqBinFetchHandler::delQueue(CwxMqTss* pTss)
{
    int iRet = CWX_MQ_ERR_SUCCESS;
    char const* queue_name = NULL;
    char const* user = NULL;
    char const* passwd = NULL;
    char const* auth_user = NULL;
    char const* auth_passwd = NULL;
    do
    {
        if (!m_recvMsgData)
        {
            strcpy(pTss->m_szBuf2K, "No data.");
            CWX_DEBUG((pTss->m_szBuf2K));
            iRet = CWX_MQ_ERR_NO_MSG;
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
        if (m_pApp->getConfig().getMq().m_mq.getUser().length())
        {
            if ((m_pApp->getConfig().getMq().m_mq.getUser() != auth_user) ||
                (m_pApp->getConfig().getMq().m_mq.getPasswd() != auth_passwd))
            {
                iRet = CWX_MQ_ERR_FAIL_AUTH;
                strcpy(pTss->m_szBuf2K, "No auth");
                break;
            }
        }
        iRet = m_pApp->getQueueMgr()->authQueue(string(queue_name), string(user), string(passwd));
        if (0 == iRet)
        {//���в�����
            iRet = CWX_MQ_ERR_NO_QUEUE;
			CwxCommon::snprintf(pTss->m_szBuf2K,  2047, "Queue[%s] doesn't exists", queue_name);
            break;

        }
        ///���Ȩ�޴����򷵻ش�����Ϣ
        if (-1 == iRet)
        {
            iRet = CWX_MQ_ERR_FAIL_AUTH;
            strcpy(pTss->m_szBuf2K, "No auth");
            break;
        }
        iRet = m_pApp->getQueueMgr()->delQueue(string(queue_name), pTss->m_szBuf2K);
        if (1 == iRet)
        {//�ɹ�
            iRet = CWX_MQ_ERR_SUCCESS;
            break;
        }
        else if (0 == iRet)
        {//exist
            iRet = CWX_MQ_ERR_QUEUE_EXIST;
            strcpy(pTss->m_szBuf2K, "Queue exists");
            break;
        }
        else
        {//�ڲ�����
            iRet = CWX_MQ_ERR_INNER_ERR;
            break;
        }
    }while(0);

    CwxMsgBlock* block = NULL;
    iRet = CwxMqPoco::packDelQueueReply(pTss->m_pWriter, block, iRet, pTss->m_szBuf2K, pTss->m_szBuf2K);
    if (CWX_MQ_ERR_SUCCESS != iRet)
    {
        CWX_ERROR(("Failure to pack del-queue-reply, err:%s", pTss->m_szBuf2K));
        return -1;
    }

    block->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_FETCH);
    block->send_ctrl().setHostId(0);
    block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(block))
    {///�ŷ��Ͷ���ʧ��
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
    if (bBinlog)
    {///�����binlog����ʱ��Ҫ������ϵ�ʱ��֪ͨ
        if (bClose) ///�Ƿ�ر�����
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FAIL_NOTICE | CwxMsgSendCtrl::FINISH_NOTICE|CwxMsgSendCtrl::CLOSE_NOTICE);
        else 
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FAIL_NOTICE | CwxMsgSendCtrl::FINISH_NOTICE);
    }
    else
    {
        if (bClose)
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
        else
            msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    }
    ///����Ϣ�ŵ����ӵķ��Ͷ��еȴ�����
    if (!putMsg(msg))
    {///�ŷ��Ͷ���ʧ��
        CWX_ERROR(("Failure to reply fetch mq"));
        ///�����binlog����Ҫ����Ϣ���ص�mq manager
        if (bBinlog) 
            backMq(pTss);
        else///�����ͷ���Ϣ
            CwxMsgBlockAlloc::free(msg);
        return -1;
    }
    return 0;
}

void CwxMqBinFetchHandler::backMq(CwxMqTss* pTss)
{
    int iRet = 0;
    if (m_conn.m_ullSendSid)
    {
        if (!m_conn.m_bSent)
        {//��ʱ������commit���У�Ҳ�����Ƿ�commit���С�
            iRet = m_pApp->getQueueMgr()->endSendMsg(m_conn.m_strQueueName,
                m_conn.m_ullSendSid,
                false,
                pTss->m_szBuf2K);
        }
        else
        {//��ʱһ����commit����
            CWX_ASSERT(m_conn.m_bCommit);
            iRet = m_pApp->getQueueMgr()->commitBinlog(m_conn.m_strQueueName,
                 m_conn.m_ullSendSid,
                 false,
                 0,
                 pTss->m_szBuf2K);
        }
        if (1 != iRet)
        {
            char szSid[32];
            if (0 == iRet)
            {///��Ϣ�Ѿ���ʱ
                CWX_DEBUG(("Sid[%s] is timeout, queue[%s]",
                    m_conn.m_strQueueName.c_str(),
                    CwxCommon::toString(m_conn.m_ullSendSid, szSid, 10)));
            }
            else if(-1 == iRet)
            {///���д���
                CWX_ERROR(("Failure to back queue[%s]'s message, err:%s",
                    m_conn.m_strQueueName.c_str(),
                    pTss->m_szBuf2K));
            }
            else
            {///���в�����
                CWX_ERROR(("Failure to back queue[%s]'s message for no existing",
                    m_conn.m_strQueueName.c_str()));
            }
        }

    }
    //������ӵķ���sid
    m_conn.m_ullSendSid = 0;
    m_conn.m_bSent = true;
}

///������Ϣ��0��û����Ϣ���ͣ�1������һ����-1������ʧ��
int CwxMqBinFetchHandler::sentBinlog(CwxMqTss* pTss)
{
    CwxMsgBlock* pBlock=NULL;
    int err_no = CWX_MQ_ERR_SUCCESS;
    int iState = 0;
    iState = m_pApp->getQueueMgr()->getNextBinlog(pTss,
        m_conn.m_strQueueName,
        pBlock,
        m_conn.m_uiTimeout,
        err_no,
        m_conn.m_bCommit,
        pTss->m_szBuf2K);
    if (-1 == iState)
    {///��ȡ��Ϣʧ��
        CWX_ERROR(("Failure to read binlog ,err:%s", pTss->m_szBuf2K));
        pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_INNER_ERR, pTss->m_szBuf2K);
        if (!pBlock)
        {
            CWX_ERROR(("No memory to malloc package"));
            return -1;
        }
        ///��log����binlog��
        if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
        return 1;
    }
    else if (0 == iState) ///�Ѿ����
    {
        if (!m_conn.m_bBlock)
        {///�������block���ͣ��ظ�û����Ϣ
            pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_NO_MSG, "No message");
            if (!pBlock)
            {
                CWX_ERROR(("No memory to malloc package"));
                return -1;
            }
            if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
            return 1;
        }
        //δ��ɣ��ȴ�block
        return 0;
    }
    else if (1 == iState)
    {///��ȡ��һ����Ϣ
        m_conn.m_ullSendSid = pBlock->event().m_ullArg;
        m_conn.m_bSent = false;
        if (0 != replyFetchMq(pTss, pBlock, true, false))
        {
            return -1;
        }
        return 1;
    }
    else if (2 == iState)
    {//δ���
        return 0;
    }
    //��ʱ��iStateΪ-2
    //no queue
    CWX_ERROR(("Not find queue:%s", m_conn.m_strQueueName.c_str()));
	string strErr = string("No queue:") + m_conn.m_strQueueName;
    pBlock = packEmptyFetchMsg(pTss, CWX_MQ_ERR_NO_QUEUE, strErr.c_str());
    if (!pBlock)
    {
        CWX_ERROR(("No memory to malloc package"));
        return -1;
    }
    if (0 != replyFetchMq(pTss, pBlock, false, false))  return -1;
    return 1;
}
