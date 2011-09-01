#ifndef __CWX_MQ_BIN_RECV_HANDLER_H__
#define __CWX_MQ_BIN_RECV_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxMqMacro.h"
#include "CwxMqTss.h"
class CwxMqApp;


///Dispatch master�����յ���binЭ���binlog handler
class CwxMqBinRecvHandler: public CwxCmdOp
{
public:
    ///���캯��
    CwxMqBinRecvHandler(CwxMqApp* pApp):m_pApp(pApp)
    {
        m_unzipBuf = NULL;
        m_uiBufLen = 0;
    }
    ///��������
    virtual ~CwxMqBinRecvHandler()
    {
        if (m_unzipBuf) delete [] m_unzipBuf;
    }
public:
    ///���ӽ�������Ҫά�����������ݵķַ�
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///���ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///�����յ�binlog���¼�
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///����ͬ��dispatch����Ҫ���ͬ���ĳ�ʱ
    virtual int onTimeoutCheck(CwxMsgBlock*& msg, CwxTss* pThrEnv);
private:
    ///-1:ʧ�ܣ�0���ɹ�
    int commit(char* szErr2K);
    //��ȡunzip��buf
    bool prepareUnzipBuf();
private:
    map<CWX_UINT32, bool>   m_clientMap; ///<������֤��map
    CwxMqApp*       m_pApp;  ///<app����
    unsigned char*          m_unzipBuf; ///<��ѹ��buffer
    CWX_UINT32              m_uiBufLen; ///<��ѹbuffer�Ĵ�С����Ϊtrunk��20������СΪ20M��
};

#endif 
