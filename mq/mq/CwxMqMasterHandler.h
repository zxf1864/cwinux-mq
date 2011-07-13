#ifndef __CWX_MQ_MASTER_HANDLER_H__
#define __CWX_MQ_MASTER_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxMqMacro.h"
#include "CwxPackageReader.h"
#include "CwxPackageWriter.h"
#include "CwxMsgBlock.h"
#include "CwxMqTss.h"

class CwxMqApp;

///slave��master����binlog�Ĵ���handle
class CwxMqMasterHandler : public CwxCmdOp
{
public:
    enum
    {
        RECONN_MASTER_DELAY_SECOND = 4
    };
public:
    ///���캯��
    CwxMqMasterHandler(CwxMqApp* pApp):m_pApp(pApp)
    {
        m_uiConnId = 0;
        m_unzipBuf = NULL;
        m_uiBufLen = 0;
		m_bSync = false;
		m_strMasterErr = "No connnect"; ///<û������
    }
    ///��������
    virtual ~CwxMqMasterHandler()
    {
        if (m_unzipBuf) delete [] m_unzipBuf;
    }
public:
    ///���ӽ�������Ҫ��master����sid
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///master�����ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��������master����Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
public:
    CWX_UINT32 getMasterConnId() const
    {
        return m_uiConnId;
    }
	string const& getMasterErr() const
	{
		return m_strMasterErr;
	}
	bool isSync() const
	{
		return m_bSync;
	}
private:
    //0���ɹ���-1��ʧ��
    int saveBinlog(CwxMqTss* pTss,
        char const* szBinLog,
        CWX_UINT32 uiLen,
        CWX_UINT64& ullSid);
    bool checkSign(char const* data,
        CWX_UINT32 uiDateLen,
        char const* szSign,
        char const* sign);
    //��ȡunzip��buf
    bool prepareUnzipBuf();
private:
    CwxMqApp*               m_pApp;  ///<app����
    CWX_UINT32              m_uiConnId; ///<master������ID
    CwxPackageReader        m_reader; ///<�����reader
    unsigned char*          m_unzipBuf; ///<��ѹ��buffer
    CWX_UINT32              m_uiBufLen; ///<��ѹbuffer�Ĵ�С����Ϊtrunk��20������СΪ20M��
	bool					m_bSync; ///<�Ƿ���ͬ��״̬
	string					m_strMasterErr; ///<master�Ĵ�����Ϣ
};

#endif 
