#ifndef __CWX_MPROXY_APP_H__
#define __CWX_MPROXY_APP_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxAppFramework.h"
#include "CwxMproxyConfig.h"
#include "CwxMproxyRecvHandler.h"
#include "CwxMproxyMqHandler.h"
#include "CwxMproxyTask.h"
#include "CwxThreadPoolEx.h"

#define CWX_MPROXY_APP_VERSION "1.3.1"
#define CWX_MPROXY_MODIFY_DATE "2010-11-07"

CWINUX_USING_NAMESPACE;

///echo��ѹ������app
class CwxMproxyApp : public CwxAppFramework
{
public:
    enum
    {
        LOG_FILE_SIZE = 30, ///<ÿ��ѭ��������־�ļ���MBTYE
        LOG_FILE_NUM = 7,///<ѭ����־�ļ�������
        SVR_TYPE_RECV = CwxAppFramework::SVR_TYPE_USER_START, ///<������Ϣ�Ľ���
        SVR_TYPE_MQ = SVR_TYPE_RECV + 1///<������Ϣ��mq�Ļظ�
    };
    ///���캯��
	CwxMproxyApp();
    ///��������
	virtual ~CwxMproxyApp();
    //��ʼ��app, -1:failure, 0 success;
    virtual int init(int argc, char** argv);
public:
    //ʱ����Ӧ����
    virtual void onTime(CwxTimeValue const& current);
    //�ź���Ӧ����
    virtual void onSignal(int signum);
    //���ӽ�������
    virtual int onConnCreated(CwxAppHandler4Msg& conn, bool& bSuspendConn, bool& bSuspendListen);
    //�յ���Ϣ����Ӧ����
    virtual int onRecvMsg(CwxMsgBlock* msg, CwxAppHandler4Msg const& conn, CwxMsgHead const& header, bool& bSuspendConn);
    //���ӹر�
    virtual int onConnClosed(CwxAppHandler4Msg const& conn);
    //��Ϣ�������
    virtual CWX_UINT32 onEndSendMsg(CwxMsgBlock*& msg,
        CwxAppHandler4Msg const& conn);
    //��Ϣ����ʧ��
    virtual void onFailSendMsg(CwxMsgBlock*& msg);
public:
    CwxMproxyConfig const& getConfig() const
    {
        return m_config;
    }

    CWX_UINT32 getNextTaskId()
    {
        CwxMutexGuard<CwxMutexLock>  lock(&m_lock);
        m_uiTaskId++;
        if (!m_uiTaskId) m_uiTaskId = 1;
        return m_uiTaskId;
    }

    CWX_UINT32 getMqConnId() const { return m_uiMqConnId; }
protected:
    //init the Enviroment before run.0:success, -1:failure.
	virtual int initRunEnv();
    virtual void destroy();
    ///����recv���ӵ�����
    static int setRecvSockAttr(CWX_HANDLE handle, void* arg);
    ///����mq���ӵ�����
    static int setMqSockAttr(CWX_HANDLE handle, void* arg);

private:
    CwxMproxyConfig                m_config; ///<�����ļ�����
    CwxMproxyRecvHandler*          m_pRecvHandle; ///���������Ϣ��handler
    CwxMproxyMqHandler*            m_pMqHandle; ///<����mq��Ϣ��handler
    CWX_UINT32                     m_uiTaskId; ///<���͸�mq����Ϣ��taskid
    CwxMutexLock                   m_lock; ///<m_uiTaskId�ı�����
    CwxThreadPoolEx*            m_threadPool;///<�̳߳ض���
    CWX_UINT32                     m_uiMqConnId;
};

#endif

