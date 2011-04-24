#include "CwxMqMcRecvHandler.h"
#include "CwxMqApp.h"

///���ӽ�������Ҫά�����������ݵķַ�
int CwxMqMcRecvHandler::onConnCreated(CwxMsgBlock*& msg, CwxTss* )
{
    ///���ӱ�����벻����
    CWX_ASSERT(m_clientMap.find(msg->event().getConnId()) == m_clientMap.end());
    m_clientMap[msg->event().getConnId()] = false;
    CWX_DEBUG(("Add recv conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///���ӹرպ���Ҫ������
int CwxMqMcRecvHandler::onConnClosed(CwxMsgBlock*& msg, CwxTss* )
{
    map<CWX_UINT32, bool>::iterator iter = m_clientMap.find(msg->event().getConnId());
    ///���ӱ������
    CWX_ASSERT(iter != m_clientMap.end());
    m_clientMap.erase(iter);
    CWX_DEBUG(("remove recv conn for conn-id[%u]", msg->event().getConnId()));
    return 1;
}

///echo����Ĵ�����
int CwxMqMcRecvHandler::onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv)
{
    return 1;
}



