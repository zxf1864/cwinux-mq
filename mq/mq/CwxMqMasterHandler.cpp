#include "CwxMqMasterHandler.h"
#include "CwxMqApp.h"
#include "CwxZlib.h"

///���ӽ�������Ҫ��master����sid
int CwxMqMasterHandler::onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    ///��������ID
    m_uiConnId = msg->event().getConnId();
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    ///������master����sid��ͨ�����ݰ�
    CwxMsgBlock* pBlock = NULL;
    int ret = CwxMqPoco::packReportData(pTss->m_pWriter,
        pBlock,
        0,
        m_pApp->getBinLogMgr()->getMaxSid(),
        false,
        m_pApp->getConfig().getCommon().m_uiChunkSize,
        m_pApp->getConfig().getCommon().m_uiWindowSize,
        m_pApp->getConfig().getSlave().m_strSubScribe.c_str(),
        m_pApp->getConfig().getSlave().m_master.getUser().c_str(),
        m_pApp->getConfig().getSlave().m_master.getPasswd().c_str(),
        m_pApp->getConfig().getSlave().m_strSign.c_str(),
        m_pApp->getConfig().getSlave().m_bzip,
        pTss->m_szBuf2K);
    if (ret != CWX_MQ_ERR_SUCCESS)
    {///���ݰ�����ʧ��
        CWX_ERROR(("Failure to create report package, err:%s", pTss->m_szBuf2K));
        m_pApp->noticeCloseConn(m_uiConnId); ///�ر����ӣ�����ִ��ͬ��
        m_uiConnId = 0;
    }
    else
    {
        ///������Ϣ
        pBlock->send_ctrl().setConnId(m_uiConnId);
        pBlock->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_MASTER);
        pBlock->send_ctrl().setHostId(0);
        pBlock->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
        if (0 != m_pApp->sendMsgByConn(pBlock))
        {//connect should be closed
            CWX_ERROR(("Failure to report sid to master"));
            CwxMsgBlockAlloc::free(pBlock);
        }
    }
	m_strMasterErr = "No report to master";
    return 1;
}

///master�����ӹرպ���Ҫ������
int CwxMqMasterHandler::onConnClosed(CwxMsgBlock*& , CwxTss* )
{
    CWX_ERROR(("Master is closed."));
    m_uiConnId = 0;
	//ˢ�¹�������������
	char szErr2K[2048];
	if (0 != m_pApp->getBinLogMgr()->commit(true, szErr2K))
	{
		CWX_ERROR(("Failure to commit binlog data, err:%s", szErr2K));
	}
	m_bSync = false;
	m_strMasterErr = "No connnect"; ///<û������
    return 1;
}

///��������master����Ϣ
int CwxMqMasterHandler::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    int iRet = 0;
    CWX_UINT32 i = 0;
    if (!m_uiConnId) return 1;
    CwxMqTss* pTss = (CwxMqTss*)pThrEnv;
    //SID����Ļظ�����ʱ��һ���Ǳ���ʧ��
    if (CwxMqPoco::MSG_TYPE_SYNC_REPORT_REPLY == msg->event().getMsgHeader().getMsgType())
    {
        ///��ʱ���Զ˻�ر�����
        CWX_UINT64 ullSid = 0;
        char const* szMsg = NULL;
        int ret = 0;
		m_bSync = false;
        if (!msg)
        {
			m_strMasterErr = "recieved report reply is empty."; ///<û������
            CWX_ERROR((m_strMasterErr.c_str()));
            return 1;
        }

        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::parseReportDataReply(pTss->m_pReader,
            msg,
            ret,
            ullSid,
            szMsg,
            pTss->m_szBuf2K))
        {
			m_strMasterErr = "Failure to parse report-reply msg from master, err=";
			m_strMasterErr += pTss->m_szBuf2K;
            CWX_ERROR((m_strMasterErr.c_str()));
            return 1;
        }
		CwxCommon::snprintf(pTss->m_szBuf2K, "Failure to sync message from master, ret=%d, err=%s", ret, szMsg);
		m_strMasterErr = pTss->m_szBuf2K;
		CWX_ERROR((pTss->m_szBuf2K));
        return 1;
    }
    else if (CwxMqPoco::MSG_TYPE_SYNC_DATA == msg->event().getMsgHeader().getMsgType())
    {///binlog����
        
        CWX_UINT64 ullSid;
        unsigned long ulUnzipLen = 0;
        bool bZip = msg->event().getMsgHeader().isAttr(CwxMsgHead::ATTR_COMPRESS);

		if (!m_bSync)
		{
			m_bSync = true;
			m_strMasterErr.erase();
		}

        if (!msg)
        {
            CWX_ERROR(("recieved sync data is empty."));
            m_pApp->noticeCloseConn(m_uiConnId);///��ʱ�ر����ӣ�����ͬ��
            m_uiConnId = 0;
            return 1;
        }

        //�ж��Ƿ�ѹ������
        if (bZip)
        {//ѹ�����ݣ���Ҫ��ѹ
            //����׼����ѹ��buf
            if (!prepareUnzipBuf())
            {
                CWX_ERROR(("Failure to prepare unzip buf, size:%u", m_uiBufLen));
                m_pApp->noticeCloseConn(m_uiConnId);///��ʱ�ر����ӣ�����ͬ��
                m_uiConnId = 0;
                return 1;
            }
            ulUnzipLen = m_uiBufLen;
            //��ѹ
            if (!CwxZlib::unzip(m_unzipBuf, ulUnzipLen, (const unsigned char*)msg->rd_ptr(), msg->length()))
            {
                CWX_ERROR(("Failure to unzip recv msg, msg size:%u, buf size:%u", msg->length(), m_uiBufLen));
                m_pApp->noticeReconnect(m_uiConnId, 2000); ///��ʱ2��������
                m_uiConnId = 0;
                return 1;
            }
        }

        //�������chunk���͵ķ���
        if (!m_pApp->getConfig().getCommon().m_uiChunkSize)
        {
            if (bZip)
            {
                iRet = saveBinlog(pTss,
                    (char*)m_unzipBuf,
                    ulUnzipLen,
                    ullSid);
            }
            else
            {
                iRet = saveBinlog(pTss,
                    msg->rd_ptr(),
                    msg->length(),
                    ullSid);
            }
        }
        else
        {
            if (bZip)
            {
                if (!m_reader.unpack((char*)m_unzipBuf, ulUnzipLen, false, true))
                {
                    CWX_ERROR(("Failure to unpack master multi-binlog, err:%s", m_reader.getErrMsg()));
                    m_pApp->noticeReconnect(m_uiConnId, 2000); ///��ʱ2��������
                    m_uiConnId = 0;
                    return 1;
                }
            }
            else
            {
                if (!m_reader.unpack(msg->rd_ptr(), msg->length(), false, true))
                {
                    CWX_ERROR(("Failure to unpack master multi-binlog, err:%s", m_reader.getErrMsg()));
                    m_pApp->noticeReconnect(m_uiConnId, 2000); ///��ʱ2��������
                    m_uiConnId = 0;
                    return 1;
                }
            }
            //���ǩ��
            int bSign = 0;
            if (m_pApp->getConfig().getSlave().m_strSign.length())
            {
                CwxKeyValueItem const* pItem = m_reader.getKey(m_pApp->getConfig().getSlave().m_strSign.c_str());
                if (pItem)
                {//����ǩ��key
                    if (!checkSign(m_reader.getMsg(),
                        pItem->m_szKey - CwxPackage::getKeyOffset() - m_reader.getMsg(),
                        pItem->m_szData,
                        m_pApp->getConfig().getSlave().m_strSign.c_str()))
                    {
                        CWX_ERROR(("Failure to check %s sign", m_pApp->getConfig().getSlave().m_strSign.c_str()));
                        m_pApp->noticeReconnect(m_uiConnId, 2000); ///��ʱ2��������
                        m_uiConnId = 0;
                        return 1;
                    }
                    bSign = 1;
                }
            }
            for (i=0; i<m_reader.getKeyNum() - bSign; i++)
            {
                if(0 != strcmp(m_reader.getKey(i)->m_szKey, CWX_MQ_M))
                {
                    CWX_ERROR(("Master multi-binlog's key must be:%s, but:%s", CWX_MQ_M, m_reader.getKey(i)->m_szKey));
                    iRet = -1;
                }
                if (-1 == saveBinlog(pTss,
                    m_reader.getKey(i)->m_szData,
                    m_reader.getKey(i)->m_uiDataLen,
                    ullSid))
                {
                    iRet = -1;
                    break;
                }
            }
            if (0 == i)
            {
                CWX_ERROR(("Master multi-binlog's key hasn't key"));
                iRet = -1;
            }
        }
        if (-1 == iRet)
        {
            m_pApp->noticeCloseConn(m_uiConnId); ///�ر�����
            m_uiConnId = 0;
            return 1;
        }
        //�ظ�������
        CwxMsgBlock* reply_block = NULL;
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packSyncDataReply(pTss->m_pWriter,
            reply_block,
            msg->event().getMsgHeader().getTaskId(),
            ullSid,
            pTss->m_szBuf2K))
        {
            CWX_ERROR(("Failure to pack sync data reply, errno=%s", pTss->m_szBuf2K));
            m_pApp->noticeCloseConn(m_uiConnId);
            m_uiConnId = 0;
            return 1;
        }
        reply_block->send_ctrl().setConnId(m_uiConnId);
        reply_block->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_MASTER);
        reply_block->send_ctrl().setHostId(0);
        reply_block->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
        if (0 != m_pApp->sendMsgByConn(reply_block))
        {//connect should be closed
            CWX_ERROR(("Failure to send sync data reply to master"));
            CwxMsgBlockAlloc::free(reply_block);
            m_pApp->noticeCloseConn(m_uiConnId);
            return 1;
        }
    }else
    {
        CWX_ERROR(("Unknow msg type[%u]", msg->event().getMsgHeader().getMsgType()));
        m_pApp->noticeReconnect(m_uiConnId, RECONN_MASTER_DELAY_SECOND * 1000);
        m_uiConnId = 0;
    }
    return 1;
}


//0���ɹ���-1��ʧ��
int CwxMqMasterHandler::saveBinlog(CwxMqTss* pTss,
                                   char const* szBinLog,
                                   CWX_UINT32 uiLen, 
                                   CWX_UINT64& ullSid)
{
    CWX_UINT32 ttTimestamp;
    CWX_UINT32 uiGroup;
    CWX_UINT32 uiType;
    CwxKeyValueItem const* data;
    ///��ȡbinlog������
    if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::parseSyncData(pTss->m_pReader, 
        szBinLog,
        uiLen,
        ullSid,
        ttTimestamp,
        data,
        uiGroup,
        uiType,
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to parse binlog from master, err=%s", pTss->m_szBuf2K));
        return -1;
    }
    //add to binlog
    pTss->m_pWriter->beginPack();
    pTss->m_pWriter->addKeyValue(CWX_MQ_DATA, data->m_szData, data->m_uiDataLen, data->m_bKeyValue);
    pTss->m_pWriter->pack();
    if (0 !=m_pApp->getBinLogMgr()->append(ullSid,
        (time_t)ttTimestamp,
        uiGroup,
        uiType,
        pTss->m_pWriter->getMsg(),
        pTss->m_pWriter->getMsgSize(),
        pTss->m_szBuf2K))
    {
        CWX_ERROR(("Failure to append binlog to binlog mgr, err=%s", pTss->m_szBuf2K));
        return -1;
    }
    return 0;
}

bool CwxMqMasterHandler::checkSign(char const* data,
               CWX_UINT32 uiDateLen,
               char const* szSign,
               char const* sign)
{
    if (!sign) return true;
    if (strcmp(sign, CWX_MQ_CRC32) == 0)//CRC32ǩ��
    {
        CWX_UINT32 uiCrc32 = CwxCrc32::value(data, uiDateLen);
        if (memcmp(&uiCrc32, szSign, sizeof(uiCrc32)) == 0) return true;
        return false;
    }
    else if (strcmp(sign, CWX_MQ_MD5)==0)//md5ǩ��
    {
        CwxMd5 md5;
        unsigned char szMd5[16];
        md5.update((const unsigned char*)data, uiDateLen);
        md5.final(szMd5);
        if (memcmp(szMd5, szSign, 16) == 0) return true;
        return false;
    }
    return true;
}
bool CwxMqMasterHandler::prepareUnzipBuf()
{
    if (!m_unzipBuf)
    {
        m_uiBufLen = m_pApp->getConfig().getCommon().m_uiChunkSize * 20;
        if (m_uiBufLen < 20 * 1024 * 1024) m_uiBufLen = 20 * 1024 * 1024;
        m_unzipBuf = new unsigned char[m_uiBufLen];
    }
    return m_unzipBuf!=NULL;
}
