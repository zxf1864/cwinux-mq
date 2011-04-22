#ifndef __CWX_MQ_MC_FETCH_HANDLER_H__
#define __CWX_MQ_MC_FETCH_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/

#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxDTail.h"
#include "CwxSTail.h"
#include "CwxTypePoolEx.h"
#include "CwxBinLogMgr.h"
#include "CwxMqDef.h"
#include "CwxMqQueueMgr.h"
#include "CwxAppHandler4Channel.h"

class CwxMqApp;

///
class CwxMqMcFetchHandler: public CwxAppHandler4Channel
{
public:
    ///���캯��
    CwxMqMcFetchHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
    {
        m_pApp = pApp;
        m_bHaveWaiting = false;
    }
    ///��������
    virtual ~CwxMqMcFetchHandler()
    {

    }
public:
    ///���ӽ�������Ҫά�����������ݵķַ�
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///���ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///�������Էַ��Ļظ���Ϣ��ͬ��״̬������Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///����binlog������ϵ���Ϣ
    virtual int onEndSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///������ʧ�ܵ�binlog
    virtual int onFailSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///����������͵���Ϣ
    virtual int onUserEvent(CwxMsgBlock*& msg, CwxTss* pThrEnv);
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
