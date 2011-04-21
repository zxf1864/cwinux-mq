#ifndef __CWX_MPROXY_RECV_HANDLER_H__
#define __CWX_MPROXY_RECV_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxTss.h"
#include "CwxGlobalMacro.h"
#include "CwxMqTss.h"

CWINUX_USING_NAMESPACE

class CwxMproxyApp;
///����mq��Ϣ�Ľ���handle
class CwxMproxyRecvHandler : public CwxCmdOp 
{
public:
    ///���캯��
    CwxMproxyRecvHandler(CwxMproxyApp* pApp):m_pApp(pApp)
    {
    }
    ///��������
    virtual ~CwxMproxyRecvHandler()
    {
    }
public:
    ///����mq��Ϣ�ĺ���
    virtual int onRecvMsg(CwxMsgBlock*& msg,  CwxTss* pThrEnv);
    //�������ӹرյ���Ϣ
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    //��ʱ��ֵ���Ϣ
    virtual int onTimeoutCheck(CwxMsgBlock*& msg, CwxTss* pThrEnv);
public:
    //�ظ������mq��Ϣ
    static void reply(CwxMproxyApp* app, CwxMsgBlock* msg, CWX_UINT32 uiConnId);

private:
    CWX_UINT32 isAuth(CwxMqTss* pTss, CWX_UINT32 uiGroup, char const* szUser, char const* szPasswd);
private:
    CwxMproxyApp*     m_pApp;  ///<app����
};

#endif 
