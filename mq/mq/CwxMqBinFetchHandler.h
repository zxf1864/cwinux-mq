#ifndef __CWX_MQ_BIN_FETCH_HANDLER_H__
#define __CWX_MQ_BIN_FETCH_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
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
        m_recvMsgData = NULL;

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
    ///������Ϣ��0���ɹ���-1��ʧ��
    int recvMessage(CwxMqTss* pTss);
    ///packһ��������mq��Ϣ�����Ϣ����
    CwxMsgBlock* packEmptyFetchMsg(CwxMqTss* pTss,
        int iRet, ///<״̬��
        char const* szErrMsg ///<������Ϣ
        );
    ///������Ϣ��0���ɹ���-1������ʧ��
    int replyFetchMq(CwxMqTss* pTss,
        CwxMsgBlock* msg,  ///<��Ϣ��
        bool bBinlog = true, ///<msg�Ƿ�Ϊbinlog��������ʧ��ʱ��Ҫ����
        bool bClose=false ///<�Ƿ��������Ҫ�ر�����
        );
    //��һ������ʧ�ܵ���Ϣ��������Ϣ����
    void backMq(CWX_UINT64 ullSid, CwxMqTss* pTss);
    ///������Ϣ��0��û����Ϣ���ͣ�1������һ����-1������ʧ��
    int sentBinlog(CwxMqTss* pTss);
    ///fetch mq,����ֵ,0���ɹ���-1��ʧ��
    int fetchMq(CwxMqTss* pTss);
    ///create queue,����ֵ,0���ɹ���-1��ʧ��
    int createQueue(CwxMqTss* pTss);
    ///del queue,����ֵ,0���ɹ���-1��ʧ��
    int delQueue(CwxMqTss* pTss);
private:
    CwxMqApp*              m_pApp;  ///<app����
    CwxMqFetchConn         m_conn; ///<mq fetch������
    CwxMsgHead             m_header; ///<��Ϣͷ
    char                   m_szHeadBuf[CwxMsgHead::MSG_HEAD_LEN + 1]; ///<��Ϣͷ��buf
    CWX_UINT32             m_uiRecvHeadLen; ///<recieved msg header's byte number.
    CWX_UINT32             m_uiRecvDataLen; ///<recieved data's byte number.
    CwxMsgBlock*           m_recvMsgData; ///<the recieved msg data
};

#endif 
