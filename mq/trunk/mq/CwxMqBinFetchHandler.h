#ifndef __CWX_MQ_BIN_FETCH_HANDLER_H__
#define __CWX_MQ_BIN_FETCH_HANDLER_H__
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
#include "CwxAppChannel.h"

class CwxMqApp;

class CwxMqBinFetchHandler: public CwxAppHandler4Channel
{
public:
    ///���캯��
    CwxMqBinFetchHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
    {
        m_pApp = pApp;
        m_uiRecvHeadLen = 0;
        m_uiRecvDataLen = 0;
        m_recvMsgData = 0;
    }
    ///��������
    virtual ~CwxMqBinFetchHandler()
    {
        if (m_recvMsgData) CwxMsgBlockAlloc::free(m_recvMsgData);
    }
public:
    /**
    @brief ���ӿɶ��¼�������-1��close()�ᱻ����
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onInput();
    /**
    @brief ֪ͨ���ӹرա�
    @return 1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
    */
    virtual int onConnClosed();
    /**
    @brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onRedo();
    /**
    @brief ֪ͨ�������һ����Ϣ�ķ��͡�<br>
    ֻ����Msgָ��FINISH_NOTICE��ʱ��ŵ���.
    @param [in,out] msg ���뷢����ϵ���Ϣ��������NULL����msg���ϲ��ͷţ�����ײ��ͷš�
    @return 
    CwxMsgSendCtrl::UNDO_CONN�����޸����ӵĽ���״̬
    CwxMsgSendCtrl::RESUME_CONN�������Ӵ�suspend״̬��Ϊ���ݽ���״̬��
    CwxMsgSendCtrl::SUSPEND_CONN�������Ӵ����ݽ���״̬��Ϊsuspend״̬
    */
    virtual CWX_UINT32 onEndSendMsg(CwxMsgBlock*& msg);

    /**
    @brief ֪ͨ�����ϣ�һ����Ϣ����ʧ�ܡ�<br>
    ֻ����Msgָ��FAIL_NOTICE��ʱ��ŵ���.
    @param [in,out] msg ����ʧ�ܵ���Ϣ��������NULL����msg���ϲ��ͷţ�����ײ��ͷš�
    @return void��
    */
    virtual void onFailSendMsg(CwxMsgBlock*& msg);
private:
    ///0���ɹ���-1��ʧ��
    int recvMessage(CwxMqTss* pTss);

    CwxMsgBlock* packErrMsg(CwxMqTss* pTss,
        int iRet,
        char const* szErrMsg
        );
    ///������Ϣ��0���ɹ���-1������ʧ��
    int reply(CwxMsgBlock* msg,
        CwxMqQueue* pQueue,
        int ret,
        bool bClose=false);
    //��һ������ʧ�ܵ���Ϣ��������Ϣ����
    void back(CwxMsgBlock* msg);
    ///������Ϣ��0��û����Ϣ���ͣ�1������һ����-1������ʧ��
    int sentBinlog(CwxMqTss* pTss, CwxMqFetchConn * pConn);
private:
    CwxMqApp*     m_pApp;  ///<app����
    CwxMqFetchConn     m_conn; ///<mq fetch������
    CwxMsgHead             m_header;
    char                   m_szHeadBuf[CwxMsgHead::MSG_HEAD_LEN];
    CWX_UINT32             m_uiRecvHeadLen; ///<recieved msg header's byte number.
    CWX_UINT32             m_uiRecvDataLen; ///<recieved data's byte number.
    CwxMsgBlock*           m_recvMsgData; ///<the recieved msg data
};

#endif 
