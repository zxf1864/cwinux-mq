#ifndef __CWX_MQ_MC_ASYNC_HANDLER_H__
#define __CWX_MQ_MC_ASYNC_HANDLER_H__
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
class CwxMqMcAsyncHandler:public CwxAppHandler4Channel
{
public:
    ///���캯��
    CwxMqMcAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel);
    ///��������
    virtual ~CwxMqMcAsyncHandler();
private:
    CwxMqApp*             m_pApp;  ///<app����
};

#endif 
