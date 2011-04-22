#ifndef __CWX_MQ_BIN_ASYNC_HANDLER_H__
#define __CWX_MQ_BIN_ASYNC_HANDLER_H__
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
#include "CwxAppHandler4Channel.h"
#include "CwxAppChannel.h"

class CwxMqApp;

///�첽binlog�ַ�����Ϣ����handler
class CwxMqBinAsyncHandler : public CwxAppHandler4Channel
{
public:
    ///���캯��
    CwxMqBinAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel);
    ///��������
    virtual ~CwxMqBinAsyncHandler();
public:
    /**
    @brief ���ӿɶ��¼�������-1��close()�ᱻ����
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onInput();
    /**
    @brief ֪ͨ���ӹرա�
    @return �����������ӣ�1������engine���Ƴ�ע�᣻0������engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
    */
    virtual int onConnClosed();
    /**
    @brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onRedo();

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
