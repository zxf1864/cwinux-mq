#ifndef __CWX_MPROXY_RECV_HANDLER_H__
#define __CWX_MPROXY_RECV_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
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
        m_unzipBuf = NULL;
        m_uiBufLen = 0;
    }
    ///��������
    virtual ~CwxMproxyRecvHandler()
    {
        if (m_unzipBuf) delete [] m_unzipBuf;
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
    //��ȡunzip��buf
    bool prepareUnzipBuf();
private:
    CwxMproxyApp*     m_pApp;  ///<app����
    unsigned char*          m_unzipBuf; ///<��ѹ��buffer
    CWX_UINT32              m_uiBufLen; ///<��ѹbuffer�Ĵ�С����Ϊtrunk��20������СΪ20M��
};

#endif 
