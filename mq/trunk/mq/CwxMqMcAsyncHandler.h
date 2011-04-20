#ifndef __CWX_MQ_MC_ASYNC_HANDLER_H__
#define __CWX_MQ_MC_ASYNC_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxAppAioWindow.h"
#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxMqDef.h"

class CwxMqApp;

///�첽binlog�ַ�����Ϣ����handler
class CwxMqMcAsyncHandler : public CwxCmdOp
{
public:
    ///���캯��
    CwxMqMcAsyncHandler(CwxMqApp* pApp);
    ///��������
    virtual ~CwxMqMcAsyncHandler();
public:
    ///���ӽ�������Ҫά�����������ݵķַ�
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///���ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///�������Էַ��Ļظ���Ϣ��ͬ��״̬������Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��Ϣ�������
    virtual int onEndSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��������Ϣ��������͵���Ϣ
    virtual int onUserEvent(CwxMsgBlock*& msg, CwxTss* pThrEnv);
public:
    void dispatch(CwxMqTss* pTss);
    ///0��δ���״̬��
    ///1�����״̬��
    ///-1��ʧ�ܣ�
    static int sendBinLog(CwxMqApp* pApp,
        CwxMqDispatchConn* conn,
        CwxMqTss* pTss);
private:
    void noticeContinue(CwxMqTss* pTss, CwxMqDispatchConn* conn);
private:
    CwxMqApp*             m_pApp;  ///<app����
    CwxMqDispatchConnSet* m_dispatchConns;
};

#endif 
