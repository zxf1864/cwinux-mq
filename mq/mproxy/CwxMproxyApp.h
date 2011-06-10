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

#define CWX_MPROXY_APP_VERSION "2.1.0"
#define CWX_MPROXY_MODIFY_DATE "20110609083000"

CWINUX_USING_NAMESPACE;

///echo��ѹ������app
class CwxMproxyApp : public CwxAppFramework
{
public:
    enum
    {
        MAX_MONITOR_REPLY_SIZE = 1024 * 1024,
        LOG_FILE_SIZE = 30, ///<ÿ��ѭ��������־�ļ���MBTYE
        LOG_FILE_NUM = 7,///<ѭ����־�ļ�������
        SVR_TYPE_RECV = CwxAppFramework::SVR_TYPE_USER_START, ///<������Ϣ�Ľ���
        SVR_TYPE_MQ = SVR_TYPE_RECV + 1,///<������Ϣ��mq�Ļظ�
        SVR_TYPE_MONITOR = SVR_TYPE_RECV + 2
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
    virtual int onRecvMsg(CwxMsgBlock* msg, CwxAppHandler4Msg& conn, CwxMsgHead const& header, bool& bSuspendConn);
    ///�յ���Ϣ����Ӧ����
    virtual int onRecvMsg(CwxAppHandler4Msg& conn,
        bool& bSuspendConn);
    //���ӹر�
    virtual int onConnClosed(CwxAppHandler4Msg& conn);
    //��Ϣ�������
    virtual CWX_UINT32 onEndSendMsg(CwxMsgBlock*& msg,
        CwxAppHandler4Msg& conn);
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
    void setMqConnId(CWX_UINT32 uiConnId) { m_uiMqConnId = uiConnId;}
protected:
    //init the Enviroment before run.0:success, -1:failure.
	virtual int initRunEnv();
    virtual void destroy();
    ///����recv���ӵ�����
    static int setRecvSockAttr(CWX_HANDLE handle, void* arg);
    ///����mq���ӵ�����
    static int setMqSockAttr(CWX_HANDLE handle, void* arg);
private:
    ///stats���-1����Ϊ����ر����ӣ�0�����ر�����
    int monitorStats(char const* buf, CWX_UINT32 uiDataLen, CwxAppHandler4Msg& conn);
    ///�γɼ�����ݣ����ؼ�����ݵĳ���
    CWX_UINT32 packMonitorInfo();
private:
    CwxMproxyConfig                m_config; ///<�����ļ�����
    CwxMproxyRecvHandler*          m_pRecvHandle; ///���������Ϣ��handler
    CwxMproxyMqHandler*            m_pMqHandle; ///<����mq��Ϣ��handler
    CWX_UINT32                     m_uiTaskId; ///<���͸�mq����Ϣ��taskid
    CwxMutexLock                   m_lock; ///<m_uiTaskId�ı�����
    CwxThreadPoolEx*               m_threadPool;///<�̳߳ض���
    CWX_UINT32                     m_uiMqConnId;
    char                           m_szBuf[MAX_MONITOR_REPLY_SIZE];///<�����Ϣ�Ļظ�buf
    string                         m_strStartTime; ///<����ʱ��
};

#endif

