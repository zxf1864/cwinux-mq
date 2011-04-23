#ifndef __CWX_MQ_MC_FETCH_HANDLER_H__
#define __CWX_MQ_MC_FETCH_HANDLER_H__
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

class CwxMqApp;

///
class CwxMqMcFetchHandler: public CwxAppHandler4Channel
{
public:
    ///���캯��
    CwxMqMcFetchHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
    {
        m_pApp = pApp;
    }
    ///��������
    virtual ~CwxMqMcFetchHandler()
    {

    }
public:
private:
    CwxMqApp*     m_pApp;  ///<app����
};

#endif 
