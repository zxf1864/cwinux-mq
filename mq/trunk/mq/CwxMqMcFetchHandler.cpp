#include "CwxMqMcFetchHandler.h"
#include "CwxMqApp.h"
///���캯��
CwxMqMcFetchHandler::CwxMqMcFetchHandler(CwxMqApp* pApp, CwxAppChannel* channel):CwxAppHandler4Channel(channel)
{
    m_pApp = pApp;
}
///��������
CwxMqMcFetchHandler::~CwxMqMcFetchHandler()
{

}
