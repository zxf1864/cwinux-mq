#include "CwxMqMcAsyncHandler.h"
#include "CwxMqApp.h"
///���캯��
CwxMqMcAsyncHandler::CwxMqMcAsyncHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
{
    m_pApp=pApp;
}
///��������
CwxMqMcAsyncHandler::~CwxMqMcAsyncHandler()
{
}
