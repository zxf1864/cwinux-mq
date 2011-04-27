#ifndef __CWX_MPROXY_TASK_H__
#define __CWX_MPROXY_TASK_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMproxyTask.h
@brief proxy �����task����
@author cwinux@gmail.com
@version 1.0
@date 2010-11-04
@warning
@bug
*/

#include "CwxTaskBoard.h"
#include "CwxTss.h"
#include "CwxGlobalMacro.h"
class CwxMproxyApp;

CWINUX_USING_NAMESPACE

class CwxMproxyTask : public CwxTaskBoardTask
{
public:
    enum
    {
        TASK_STATE_WAITING = TASK_STATE_USER
    };
    ///���캯��
    CwxMproxyTask(CwxMproxyApp* pApp, CwxTaskBoard* pTaskBoard):CwxTaskBoardTask(pTaskBoard),m_pApp(pApp)
    {
        m_uiReplyConnId = 0;
        m_uiMsgTaskId = 0;
        m_sndMsg = NULL;
        m_bReplyTimeout = false;
        m_bFailSend = false;
        m_uiSendConnId = 0;
        m_mqReply =NULL;
    }
    ///��������
    ~CwxMproxyTask()
    {
        if (m_sndMsg) CwxMsgBlockAlloc::free(m_sndMsg);
        if (m_mqReply) CwxMsgBlockAlloc::free(m_mqReply);
    }
public:
    /**
    @brief ֪ͨTask�Ѿ���ʱ
    @param [in] pThrEnv �����̵߳�Thread-env
    @return void
    */
    virtual void noticeTimeout(CwxTss* pThrEnv);
    /**
    @brief ֪ͨTask���յ�һ�����ݰ���
    @param [in] msg �յ�����Ϣ
    @param [in] pThrEnv �����̵߳�Thread-env
    @param [out] bConnAppendMsg �յ���Ϣ�������ϣ��Ƿ��д����յ�������Ϣ��true���ǣ�false��û��
    @return void
    */
    virtual void noticeRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv, bool& bConnAppendMsg);
    /**
    @brief ֪ͨTask���ⷢ�͵�һ�����ݰ�����ʧ�ܡ�
    @param [in] msg �յ�����Ϣ
    @param [in] pThrEnv �����̵߳�Thread-env
    @return void
    */
    virtual void noticeFailSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    /**
    @brief ֪ͨTaskͨ��ĳ�����ӣ�������һ�����ݰ���
    @param [in] msg ���͵����ݰ�����Ϣ
    @param [in] pThrEnv �����̵߳�Thread-env
    @param [out] bConnAppendMsg ������Ϣ�������ϣ��Ƿ��еȴ��ظ�����Ϣ��true���ǣ�false��û��
    @return void
    */
    virtual void noticeEndSendMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv, bool& bConnAppendMsg);
    /**
    @brief ֪ͨTask�ȴ��ظ���Ϣ��һ�����ӹرա�
    @param [in] uiSvrId �ر����ӵ�SVR-ID
    @param [in] uiHostId �ر����ӵ�HOST-ID
    @param [in] uiConnId �ر����ӵ�CONN-ID
    @param [in] pThrEnv �����̵߳�Thread-env
    @return void
    */
    virtual void noticeConnClosed(CWX_UINT32 uiSvrId, CWX_UINT32 uiHostId, CWX_UINT32 uiConnId, CwxTss* pThrEnv);
    /**
    @brief ����Task����Task����ǰ��Task��Task�Ĵ����߳���ӵ�С�
    ������ǰ��Task���Խ����Լ����첽��Ϣ�������ܴ���
    ��ʱ��Taskboard��noticeActiveTask()�ӿڵ��õġ�
    @param [in] pThrEnv �����̵߳�Thread-env
    @return 0���ɹ���-1��ʧ��
    */
    virtual int noticeActive(CwxTss* pThrEnv);
    /**
    @brief ִ��Task���ڵ��ô�APIǰ��Task��Taskboard�в����ڣ�Ҳ����˵�Ա���̲߳��ɼ���
    TaskҪô�Ǹմ���״̬��Ҫô�������ǰһ���׶εĴ����������״̬��
    ͨ���˽ӿڣ���Task�Լ������Լ���step����ת����������ϵTask�����ͼ�������̡�
    @param [in] pTaskBoard ����Task��Taskboard
    @param [in] pThrEnv �����̵߳�Thread-env
    @return void
    */
    virtual void execute(CwxTss* pThrEnv);
private:
    void reply(CwxTss* pThrEnv);
public:
    CWX_UINT32     m_uiReplyConnId; ///<�ظ�������ID
    CWX_UINT32     m_uiMsgTaskId; ///<���յ���Ϣ��TaskId
    CwxMsgBlock*   m_sndMsg; ///<���͵�mq��Ϣ
private:
    bool           m_bReplyTimeout; ///<�Ƿ�ظ���ʱ
    bool           m_bFailSend; ///<�Ƿ���ʧ��
    CWX_UINT32     m_uiSendConnId;
    CwxMsgBlock*   m_mqReply;
    CwxMproxyApp*  m_pApp; ///<app����
};


#endif
