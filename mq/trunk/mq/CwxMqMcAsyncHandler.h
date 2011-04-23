#ifndef __CWX_MQ_MC_ASYNC_HANDLER_H__
#define __CWX_MQ_MC_ASYNC_HANDLER_H__
/*
版权声明：
    本软件遵循GNU LGPL（http://www.gnu.org/copyleft/lesser.html），
    联系方式：email:cwinux@gmail.com；微博:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxAppAioWindow.h"
#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxMqDef.h"
#include "CwxAppHandler4Channel.h"
#include "CwxAppChannel.h"

class CwxMqApp;

///异步binlog分发的消息处理handler
class CwxMqMcAsyncHandler:public CwxAppHandler4Channel
{
public:
    ///构造函数
    CwxMqMcAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel);
    ///析构函数
    virtual ~CwxMqMcAsyncHandler();
private:
    CwxMqApp*             m_pApp;  ///<app对象
};

#endif 
