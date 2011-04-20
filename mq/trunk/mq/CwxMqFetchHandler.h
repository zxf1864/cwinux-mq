#ifndef __CWX_MQ_FETCH_HANDLER_H__
#define __CWX_MQ_FETCH_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxAppCommander.h"
#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxDTail.h"
#include "CwxSTail.h"
#include "CwxTypePoolEx.h"
#include "CwxBinLogMgr.h"
#include "CwxMqDef.h"
#include "CwxMqQueueMgr.h"

class CwxMqApp;



///Dispatch master�����յ���binlog handler
class CwxMqFetchHandler: public CwxAppCmdOp
{
public:
    ///���캯��
    CwxMqFetchHandler(CwxMqApp* pApp):m_pApp(pApp)
    {
        m_bHaveWaiting = false;
    }
    ///��������
    virtual ~CwxMqFetchHandler()
    {

    }
public:
    ///���ӽ�������Ҫά�����������ݵķַ�
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
    ///���ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
    ///�������Էַ��Ļظ���Ϣ��ͬ��״̬������Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
    ///����binlog������ϵ���Ϣ
    virtual int onEndSendMsg(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
    ///������ʧ�ܵ�binlog
    virtual int onFailSendMsg(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
    ///����������͵���Ϣ
    virtual int onUserEvent(CwxMsgBlock*& msg, CwxAppTss* pThrEnv);
public:
    void dispatch(CwxMqTss* pTss);
private:
    CwxMsgBlock* packErrMsg(CwxMqTss* pTss,
        int iRet,
        char const* szErrMsg
        );
    void reply(CwxMsgBlock* msg,
        CWX_UINT32 uiConnId,
        CwxMqQueue* pQueue,
        int ret,
        bool bClose=false);
    //��һ������ʧ�ܵ���Ϣ��������Ϣ����
    void back(CwxMsgBlock* msg);
    ///����������Ϣ
    void noticeContinue(CwxMqTss* pTss, CWX_UINT32 uiConnId);
    ///������Ϣ
    void sentBinlog(CwxMqTss* pTss, CwxMqFetchConn * pConn);
private:
    CwxMqApp*     m_pApp;  ///<app����
    CwxMqFetchConnSet     m_fetchConns; ///<mq fetch�����Ӽ���
    bool          m_bHaveWaiting; ///<�Ƿ��еȴ��ķ��Ͷ���
};

#endif 
