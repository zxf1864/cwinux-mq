#include "CwxMqBinAsyncHandler.h"
#include "CwxMqApp.h"
///���캯��
CwxMqBinAsyncHandler::CwxMqBinAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
{
    m_pApp = pApp;
    m_uiRecvHeadLen = 0;
    m_uiRecvDataLen = 0;
    m_recvMsgData = 0;
}
///��������
CwxMqBinAsyncHandler::~CwxMqBinAsyncHandler()
{
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    if (m_dispatch.m_pCursor) m_pApp->getBinLogMgr()->destoryCurser(m_dispatch.m_pCursor);
}

/**
@brief ���ӿɶ��¼�������-1��close()�ᱻ����
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinAsyncHandler::onInput()
{
    ///������Ϣ
    int ret = CwxAppHandler4Channel::recvPackage(getHandle(),
        m_uiRecvHeadLen,
        m_uiRecvDataLen,
        m_szHeadBuf,
        m_header,
        m_recvMsgData);
    ///���û�н�����ϣ�0����ʧ�ܣ�-1�����򷵻�
    if (1 != ret) return ret;
    ///���յ�һ�����������ݰ�
    ///��ȡ�̵߳�tssʵ��
    CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
    ///��Ϣ����
    ret = recvMessage(tss);
    ///���û���ͷŽ��յ����ݰ����ͷ�
    if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    this->m_recvMsgData = NULL;
    this->m_uiRecvHeadLen = 0;
    this->m_uiRecvDataLen = 0;
    return ret;
}

//1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
int CwxMqBinAsyncHandler::onConnClosed()
{
    ///��epoll engine���Ƴ�handler
    return -1;
}



///���ӹرպ���Ҫ������
int CwxMqBinAsyncHandler::recvMessage(CwxMqTss* pTss)
{
    CWX_UINT64 ullSid = 0;
    CWX_UINT32 uiChunk = 0;
    CWX_UINT32 uiWindow = 1;
    bool  bNewly = false;
    char const* subscribe=NULL;
    char const* user=NULL;
    char const* passwd=NULL;
    char const* sign = NULL;
    bool bzip = false;
    int iRet = CWX_MQ_ERR_SUCCESS;
    int iState = 0;
    do 
    {

        if (CwxMqPoco::MSG_TYPE_SYNC_DATA_REPLY == m_header.getMsgType())
        {
            if (!m_dispatch.m_bSync)
            { ///������Ӳ���ͬ��״̬�����Ǵ���
                strcpy(pTss->m_szBuf2K, "Client no in sync state");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_INVALID_MSG_TYPE;
                break;
            }
            if (!m_dispatch.m_sendingSid.size())
            {
                strcpy(pTss->m_szBuf2K, "Not sent binlog data");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_INVALID_MSG_TYPE;
                break;
            }
            if (!m_recvMsgData)
            {
                strcpy(pTss->m_szBuf2K, "No data.");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_NO_MSG;
                break;
            }
            ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
            iRet = CwxMqPoco::parseSyncDataReply(pTss->m_pReader,
                m_recvMsgData,
                ullSid,
                pTss->m_szBuf2K);
            if (CWX_MQ_ERR_SUCCESS != iRet)
            {
                CWX_DEBUG(("Failure to parse sync_data reply package, err:%s", pTss->m_szBuf2K));
                break;
            }
            ///��鷵�ص�sid
            if (ullSid != *m_dispatch.m_sendingSid.begin())
            {
                char szBuf1[64];
                char szBuf2[64];
                CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Reply sid[%s] is not the right sid[%s], close conn.",
                    CwxCommon::toString(ullSid, szBuf1, 10),
                    CwxCommon::toString(*m_dispatch.m_sendingSid.begin(), szBuf2, 10));
                CWX_ERROR((pTss->m_szBuf2K));
               iRet = CWX_MQ_ERR_INVALID_SID;
                break;
            }
            ///��sid�ӷַ�sid��set��ȥ��
            m_dispatch.m_sendingSid.erase(m_dispatch.m_sendingSid.begin());
            ///������һ��binlog
            iState = sendBinLog(pTss);
            if (-1 == iState)
            {
                CWX_ERROR((pTss->m_szBuf2K));
                return -1; ///�ر�����
            }
            else if (0 == iState)
            {///����continue����Ϣ
                m_pApp->getAsyncDispChannel()->regRedoHander(this);
            }
            ///����
            return 0;
        }
        else if (CwxMqPoco::MSG_TYPE_SYNC_REPORT == m_header.getMsgType())
        {
            if (!m_recvMsgData)
            {
                strcpy(pTss->m_szBuf2K, "No data.");
                CWX_DEBUG((pTss->m_szBuf2K));
                iRet = CWX_MQ_ERR_NO_MSG;
                break;
            }
            ///��ֹ�ظ�report sid����cursor���ڣ���ʾ�Ѿ������һ��
            if (m_dispatch.m_pCursor)
            {
                iRet = CWX_MQ_ERR_INVALID_MSG;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Can't report sync sid duplicatly.");
                CWX_ERROR((pTss->m_szBuf2K));
                break;
            }
            ///����ͬ��sid�ı�����Ϣ,���ȡ�����sid
            iRet = CwxMqPoco::parseReportData(pTss->m_pReader,
                m_recvMsgData,
                ullSid,
                bNewly,
                uiChunk,
                uiWindow,
                subscribe,
                user,
                passwd,
                sign,
                bzip,
                pTss->m_szBuf2K);
            if (CWX_MQ_ERR_SUCCESS != iRet)
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
                        iRet = CWX_MQ_ERR_FAIL_AUTH;
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
                        iRet = CWX_MQ_ERR_FAIL_AUTH;
                        CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Failure to auth user[%s] passwd[%s]", user, passwd);
                        CWX_DEBUG((pTss->m_szBuf2K));
                        break;
                    }
                }
            }
            string strSubcribe=subscribe?subscribe:"";
            string strErrMsg;
            if (!CwxMqPoco::parseSubsribe(subscribe, m_dispatch.m_subscribe, strErrMsg))
            {
                iRet = CWX_MQ_ERR_INVALID_SUBSCRIBE;
                CwxCommon::snprintf(pTss->m_szBuf2K, 2048, "Invalid subscribe[%s], err=%s", strSubcribe.c_str(), strErrMsg.c_str());
                CWX_DEBUG((pTss->m_szBuf2K));
                break;
            }
            m_dispatch.m_bSync = true;
            m_dispatch.m_uiChunk = uiChunk;
            m_dispatch.m_bZip = bzip;
            m_dispatch.m_strSign = sign;
            if ((m_dispatch.m_strSign != CWX_MQ_CRC32) &&
                (m_dispatch.m_strSign != CWX_MQ_MD5))
            {//���ǩ������CRC32��MD5�������
                m_dispatch.m_strSign.erase();
            }
            if (m_dispatch.m_uiChunk)
            {
                if (m_dispatch.m_uiChunk > CwxMqConfigCmn::MAX_CHUNK_SIZE_KB) m_dispatch.m_uiChunk = CwxMqConfigCmn::MAX_CHUNK_SIZE_KB;
                if (m_dispatch.m_uiChunk < CwxMqConfigCmn::MIN_CHUNK_SIZE_KB) m_dispatch.m_uiChunk = CwxMqConfigCmn::MIN_CHUNK_SIZE_KB;
                m_dispatch.m_uiChunk *= 1024;
            }
            if (!uiWindow)
                m_dispatch.m_uiWindow = CwxMqConfigCmn::DEF_WINDOW_NUM;
            else
                m_dispatch.m_uiWindow = uiWindow;
            if (m_dispatch.m_uiWindow > CwxMqConfigCmn::MAX_WINDOW_NUM) m_dispatch.m_uiWindow = CwxMqConfigCmn::MAX_WINDOW_NUM;
            if (m_dispatch.m_uiWindow < CwxMqConfigCmn::MIN_WINDOW_NUM) m_dispatch.m_uiWindow = CwxMqConfigCmn::MIN_WINDOW_NUM;
            m_dispatch.m_sendingSid.clear();

            if (bNewly)
            {///��sidΪ�գ���ȡ��ǰ���sid-1
                ullSid = m_pApp->getBinLogMgr()->getMaxSid();
                if (ullSid) ullSid--;
            }
            ///�ظ�iRet��ֵ
            iRet = CWX_MQ_ERR_SUCCESS;
            ///����binlog��ȡ��cursor
            CwxBinLogCursor* pCursor = m_pApp->getBinLogMgr()->createCurser(ullSid);
            if (!pCursor)
            {
                iRet = CWX_MQ_ERR_INNER_ERR;
                strcpy(pTss->m_szBuf2K, "Failure to create cursor");
                CWX_ERROR((pTss->m_szBuf2K));
                break;
            }
			if (ullSid && ullSid < m_pApp->getBinLogMgr()->getMinSid())
			{
				m_pApp->getBinLogMgr()->destoryCurser(pCursor);
				iRet = CWX_MQ_ERR_LOST_SYNC;
				char szBuf1[64], szBuf2[64];
				sprintf(pTss->m_szBuf2K, "Lost sync state, report sid:%s, min sid:%s",
					CwxCommon::toString(ullSid, szBuf1),
					CwxCommon::toString(m_pApp->getBinLogMgr()->getMinSid(), szBuf2));
				CWX_ERROR((pTss->m_szBuf2K));
				break;
			}
            ///����cursor
            m_dispatch.m_pCursor = pCursor;
            m_dispatch.m_ullStartSid = ullSid;
            m_dispatch.m_bSync = true;
            ///������һ��binlog
            iState = sendBinLog(pTss);
            if (-1 == iState)
            {
                CWX_ERROR((pTss->m_szBuf2K));
                return -1; ///�ر�����
            }
            else if (0 == iState)
            {///����continue����Ϣ
                m_pApp->getAsyncDispChannel()->regRedoHander(this);
            }
            ///����
            return 0;
        }
        else
        {
            ///��������Ϣ���򷵻ش���
            CwxCommon::snprintf(pTss->m_szBuf2K, 2047, "Invalid msg type:%u", m_header.getMsgType());
            iRet = CWX_MQ_ERR_INVALID_MSG_TYPE;
            CWX_ERROR((pTss->m_szBuf2K));
        }

    } while(0);
    

    ///�γ�ʧ��ʱ��Ļظ����ݰ�
    CwxMsgBlock* pBlock = NULL;
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packReportDataReply(pTss->m_pWriter,
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
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_ASYNC);
    pBlock->send_ctrl().setHostId(0);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::CLOSE_NOTICE);
    if (!this->putMsg(pBlock))
    {
        CWX_ERROR(("Failure to send msg to reciever, conn[%u]", getHandle()));
        CwxMsgBlockAlloc::free(pBlock);
        return -1;
        ///�ر�����
    }
    return 0;
}

/**
@brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
@return -1������ʧ�ܣ������close()�� 0������ɹ�
*/
int CwxMqBinAsyncHandler::onRedo()
{
    ///�ж��Ƿ��пɷ��͵���Ϣ
    if (!m_dispatch.m_sendingSid.size() ||
        (*(m_dispatch.m_sendingSid.end()--) < m_pApp->getBinLogMgr()->getMaxSid()))
    {
        CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
        ///������һ��binlog
        int iState = sendBinLog(tss);
        if (-1 == iState)
        {
            CWX_ERROR((tss->m_szBuf2K));
            return -1; ///�ر�����
        }
        else if (0 == iState)
        {///����continue����Ϣ
            m_pApp->getAsyncDispChannel()->regRedoHander(this);
        }
    }
    else
    {
        ///����redo handler
        m_pApp->getAsyncDispChannel()->regRedoHander(this);
    }
    ///����
    return 0;
}

CWX_UINT32 CwxMqBinAsyncHandler::onEndSendMsg(CwxMsgBlock*& )
{
    ///������з��ʹ���
    if (m_dispatch.m_sendingSid.size() < m_pApp->getConfig().getCommon().m_uiWindowSize)
    {
        CwxMqTss* tss = (CwxMqTss*)CwxTss::instance();
        ///������һ��binlog
        int iState = sendBinLog(tss);
        if (-1 == iState)
        {
            CWX_ERROR((tss->m_szBuf2K));
        }
    }
    return CwxMsgSendCtrl::UNDO_CONN;
}


///-1��ʧ�ܣ�0����Ч����Ϣ��1���ɹ�
int CwxMqBinAsyncHandler::packOneBinLog(CwxPackageReader* reader,
                         CwxPackageWriter* writer,
                         CwxMsgBlock*& block,
                         char const* szData,
                         CWX_UINT32  uiDataLen,
                         char* szErr2K)
{
    CwxKeyValueItem const* pItem = NULL;
    ///unpack data�����ݰ�
    if (reader->unpack(szData, uiDataLen, false,true))
    {
        ///��ȡCWX_MQ_DATA��key����Ϊ����data����
        pItem = reader->getKey(CWX_MQ_DATA);
        if (pItem)
        {
            ///�γ�binlog���͵����ݰ�
            if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncData(writer,
                block,
                0,
                m_dispatch.m_pCursor->getHeader().getSid(),
                m_dispatch.m_pCursor->getHeader().getDatetime(),
                *pItem,
                m_dispatch.m_pCursor->getHeader().getGroup(),
                m_dispatch.m_pCursor->getHeader().getType(),
                m_dispatch.m_strSign.c_str(),
                m_dispatch.m_bZip,
                szErr2K))
            {
                ///�γ����ݰ�ʧ��
                CWX_ERROR(("Failure to pack binlog package, err:%s", szErr2K));
                return -1;
            }
        }
        else
        {///��ȡ��������Ч                
            CWX_ERROR(("Can't find key[%s] in binlog, sid=%s", CWX_MQ_DATA,
                CwxCommon::toString(m_dispatch.m_pCursor->getHeader().getSid(), szErr2K)));
            return 0;
        }            
    }
    else
    {///binlog�����ݸ�ʽ���󣬲���kv
        CWX_ERROR(("Can't unpack binlog, sid=%s", CwxCommon::toString(m_dispatch.m_pCursor->getHeader().getSid(), szErr2K)));
        return 0;
    }
    return 1;
}

///-1��ʧ�ܣ����򷵻�������ݵĳߴ�
int CwxMqBinAsyncHandler::packMultiBinLog(CwxPackageReader* reader,
                           CwxPackageWriter* writer,
                           CwxPackageWriter* writer_item,
                           char const* szData,
                           CWX_UINT32  uiDataLen,
                           CWX_UINT32&  uiLen,
                           char* szErr2K)
{
    CwxKeyValueItem const* pItem = NULL;
    ///unpack data�����ݰ�
    if (reader->unpack(szData, uiDataLen, false,true))
    {
        ///��ȡCWX_MQ_DATA��key����Ϊ����data����
        pItem = reader->getKey(CWX_MQ_DATA);
        if (pItem)
        {
            ///�γ�binlog���͵����ݰ�
            if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncDataItem(writer_item,
                m_dispatch.m_pCursor->getHeader().getSid(),
                m_dispatch.m_pCursor->getHeader().getDatetime(),
                *pItem,
                m_dispatch.m_pCursor->getHeader().getGroup(),
                m_dispatch.m_pCursor->getHeader().getType(),
                szErr2K))
            {
                ///�γ����ݰ�ʧ��
                CWX_ERROR(("Failure to pack binlog package, err:%s", szErr2K));
                return -1;
            }
        }
        else
        {///��ȡ��������Ч                
            CWX_ERROR(("Can't find key[%s] in binlog, sid=%s", CWX_MQ_DATA,
                CwxCommon::toString(m_dispatch.m_pCursor->getHeader().getSid(), szErr2K)));
            return 0;
        }            
    }
    else
    {///binlog�����ݸ�ʽ���󣬲���kv
        CWX_ERROR(("Can't unpack binlog, sid=%s", CwxCommon::toString(m_dispatch.m_pCursor->getHeader().getSid(), szErr2K)));
        return 0;
    }
    if (!writer->addKeyValue(CWX_MQ_M, writer_item->getMsg(), writer_item->getMsgSize(),true))
    {
        ///�γ����ݰ�ʧ��
        CWX_ERROR(("Failure to pack binlog package, err:%s", writer->getErrMsg()));
        return -1;
    }
    uiLen = CwxPackage::getKvLen(strlen(CWX_MQ_M),  writer_item->getMsgSize());
    return 1;
}

//1�����ּ�¼��0��û�з��֣�-1������
int CwxMqBinAsyncHandler::seekToLog(CWX_UINT32& uiSkipNum, bool bSync)
{
    int iRet = 0;
    if (m_dispatch.m_bNext)
    {
        iRet = m_pApp->getBinLogMgr()->next(m_dispatch.m_pCursor);
        if (0 == iRet) return 0; ///���״̬
        if (-1 == iRet)
        {///<ʧ��
            CWX_ERROR(("Failure to seek cursor, err:%s", m_dispatch.m_pCursor->getErrMsg()));
            return -1;
        }
    }
    uiSkipNum++;
    m_dispatch.m_bNext = false;
    while (!CwxMqPoco::isSubscribe(m_dispatch.m_subscribe,
        bSync,
        m_dispatch.m_pCursor->getHeader().getGroup(),
        m_dispatch.m_pCursor->getHeader().getType()))
    {
        iRet = m_pApp->getBinLogMgr()->next(m_dispatch.m_pCursor);
        if (0 == iRet)
        {
            m_dispatch.m_bNext = true;
            return 0; ///���״̬
        }
        if (-1 == iRet)
        {///<ʧ��
            CWX_ERROR(("Failure to seek cursor, err:%s", m_dispatch.m_pCursor->getErrMsg()));
            return -1;
        }
        uiSkipNum ++;
        if (!CwxMqPoco::isContinueSeek(uiSkipNum))
        {
            return 0;///δ���״̬
        }
    }
    return 1;
}

//1���ɹ���0��̫��-1������
int CwxMqBinAsyncHandler::seekToReportSid()
{
    int iRet = 0;
    if (m_pApp->getBinLogMgr()->isUnseek(m_dispatch.m_pCursor))
    {//��binlog�Ķ�ȡcursor���գ���λ
        if (m_dispatch.m_ullStartSid < m_pApp->getBinLogMgr()->getMaxSid())
        {
            iRet = m_pApp->getBinLogMgr()->seek(m_dispatch.m_pCursor, m_dispatch.m_ullStartSid);
            if (-1 == iRet)
            {
                CWX_ERROR(("Failure to seek,  err:%s", m_dispatch.m_pCursor->getErrMsg()));
                return -1;
            }
            else if (0 == iRet)
            {
                char szBuf1[64];
                char szBuf2[64];
                CWX_DEBUG(("Should seek to sid[%s] with max_sid[[%s], but not.",
                    CwxCommon::toString(m_dispatch.m_ullStartSid, szBuf1),
                    CwxCommon::toString(m_pApp->getBinLogMgr()->getMaxSid(), szBuf2)));
                return 0;
            }
            ///���ɹ���λ�����ȡ��ǰ��¼
            m_dispatch.m_bNext = m_dispatch.m_ullStartSid == m_dispatch.m_pCursor->getHeader().getSid()?true:false;
        }
        else
        {///����Ҫͬ�����͵�sid��С�ڵ�ǰ��С��sid��������Ϊ����״̬
            return 0;///���״̬
        }
    }
    return 1;
}

///0��δ����һ��binlog��
///1��������һ��binlog��
///-1��ʧ�ܣ�
///2����������
int CwxMqBinAsyncHandler::sendBinLog(CwxMqTss* pTss)
{
    if (m_dispatch.m_uiWindow <= m_dispatch.m_sendingSid.size()) return 2;
    int iRet = 0;
    CWX_UINT32 uiDataLen;
    char* pBuf = NULL;
    CwxMsgBlock* pBlock = NULL;
    CWX_UINT32 uiSkipNum = 0;
    CWX_UINT32 uiKeyLen = 0;
    CWX_UINT32 uiTotalLen = 0;
    CWX_UINT64 ullSid = 0;
    if (m_pApp->getBinLogMgr()->isUnseek(m_dispatch.m_pCursor))
    {//��binlog�Ķ�ȡcursor���գ���λ
        if (1 != (iRet = seekToReportSid())) return iRet;
    }

    if (m_dispatch.m_uiChunk)
    {
        pTss->m_pWriter->beginPack();
    }
    while(1)
    {
        if ( 1 != (iRet = seekToLog(uiSkipNum, true))) break;
        //�����Ƶ���һ����¼λ��
        m_dispatch.m_bNext = true;
        uiDataLen = m_dispatch.m_pCursor->getHeader().getLogLen();
        ///׼��data��ȡ��buf
        pBuf = pTss->getBuf(uiDataLen);        
        ///��ȡdata
        iRet = m_pApp->getBinLogMgr()->fetch(m_dispatch.m_pCursor, pBuf, uiDataLen);
        if (-1 == iRet)
        {//��ȡʧ��
            CWX_ERROR(("Failure to fetch data, err:%s", m_dispatch.m_pCursor->getErrMsg()));
            iRet = -1;
            break;
        }
        if (!m_dispatch.m_uiChunk)
        {
            iRet = packOneBinLog(pTss->m_pReader,
                pTss->m_pWriter,
                pBlock,
                pBuf,
                uiDataLen,
                pTss->m_szBuf2K);
            if (0 == iRet) continue;
            if (1 == iRet) ullSid = m_dispatch.m_pCursor->getHeader().getSid();
            break;
        }
        else
        {
            iRet = packMultiBinLog(pTss->m_pReader, 
                pTss->m_pWriter,
                pTss->m_pItemWriter,
                pBuf,
                uiDataLen,
                uiKeyLen,
                pTss->m_szBuf2K);
            if (1 == iRet)
            {
                ullSid = m_dispatch.m_pCursor->getHeader().getSid();
                uiTotalLen += uiKeyLen;
                if (uiTotalLen >= m_dispatch.m_uiChunk) break;
            }
            if (-1 == iRet) break;
            continue;
        }
    }
    if (-1 == iRet) return -1;

    if (!m_dispatch.m_uiChunk)
    {
        if (0 == iRet) return 0;
    }
    else
    {
        if (0 == uiTotalLen) return 0;
        //add sign
        if (m_dispatch.m_strSign.length())
        {
            if (m_dispatch.m_strSign == CWX_MQ_CRC32)//CRC32ǩ��
            {
                CWX_UINT32 uiCrc32 = CwxCrc32::value(pTss->m_pWriter->getMsg(), pTss->m_pWriter->getMsgSize());
                if (!pTss->m_pWriter->addKeyValue(CWX_MQ_CRC32, (char*)&uiCrc32, sizeof(uiCrc32)))
                {
                    CWX_ERROR(("Failure to add key value, err:%s", pTss->m_pWriter->getErrMsg()));
                    return -1;
                }
            }
            else if (m_dispatch.m_strSign == CWX_MQ_MD5)//md5ǩ��
            {
                CwxMd5 md5;
                unsigned char szMd5[16];
                md5.update((unsigned char*)pTss->m_pWriter->getMsg(), pTss->m_pWriter->getMsgSize());
                md5.final(szMd5);
                if (!pTss->m_pWriter->addKeyValue(CWX_MQ_MD5, (char*)szMd5, 16))
                {
                    CWX_ERROR(("Failure to add key value, err:%s", pTss->m_pWriter->getErrMsg()));
                    return -1;
                }
            }
        }

        pTss->m_pWriter->pack();
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packMultiSyncData(0,
            pTss->m_pWriter->getMsg(), 
            pTss->m_pWriter->getMsgSize(),
            pBlock,
            m_dispatch.m_bZip,
            pTss->m_szBuf2K))
        {
            return -1;
        }
    }
    ///����svr���ͣ��������ݰ�
    pBlock->send_ctrl().setConnId(CWX_APP_INVALID_CONN_ID);
    pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_ASYNC);
    pBlock->send_ctrl().setHostId(0);
    pBlock->event().setTaskId(m_dispatch.m_pCursor->getHeader().getSid()&0xFFFFFFFF);
    pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::FINISH_NOTICE);
    if (!putMsg(pBlock))
    {
        CWX_ERROR(("Failure to send binlog"));
        CwxMsgBlockAlloc::free(pBlock);
        return -1;
    }
    m_dispatch.m_sendingSid.insert(ullSid);
    return 1; ///������һ����Ϣ
}

