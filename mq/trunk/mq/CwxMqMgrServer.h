#ifndef __CWX_MQ_MGR_SERVER_H__
#define __CWX_MQ_MGR_SERVER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html��
*/

/**
@file CwxMqMgrServer.h
@brief �����ع��������CwxMqMgrServer��Ķ���
@author cwinux@gmail.com
@version 0.1
@date 2010-09-15
@warning
@bug
*/
#include "CwxMqMacro.h"
#include "CwxAppMgrServer.h"


class CwxMqMgrServer: public CwxAppMgrServer
{
public:
    ///���캯��
    CwxMqMgrServer(CwxAppFramework* pApp):CwxAppMgrServer(pApp)
    {

    }
    ///��������
    virtual ~CwxMqMgrServer()
    {

    }
public:
    /**
    @brief ��ȡ������ϸ������Ϣ�������Ӧ����
    @param [in] msg msg�������ݰ�
    @param [in] pThrEnv �̵߳�Thread-env��
    @param [in] reply �ظ���package����ص���Ϣ��
    @return false�����ظ����ر����ӣ� true���ظ�
    */
    virtual bool onCmdRunDetail(CwxMsgBlock*& msg,
        CwxAppTss* pThrEnv,
        CwxAppMgrReply& reply);
};


#endif 
