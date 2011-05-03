#ifndef __CWX_MQ_BIN_RECV_HANDLER_H__
#define __CWX_MQ_BIN_RECV_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
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
        m_uiUnSyncLogNum = 0; ///<�ϴ��γ�sync��¼�������¼�¼��
        m_ttLastSyncTime = 0; ///<��һ���γ�sync��¼��ʱ��
    }
    ///��������
    virtual ~CwxMqBinRecvHandler()
    {

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
    ///-1:ʧ�ܣ�0�������γ�log��1���γ�һ��log
    int checkSyncLog(bool bNew, char* szErr2K);
private:
    CWX_UINT32      m_uiUnSyncLogNum; ///<�ϴ��γ�sync��¼�������¼�¼��
    time_t          m_ttLastSyncTime; ///<��һ���γ�sync��¼��ʱ��
    map<CWX_UINT32, bool>   m_clientMap; ///<������֤��map
    CwxMqApp*       m_pApp;  ///<app����
};

#endif 
