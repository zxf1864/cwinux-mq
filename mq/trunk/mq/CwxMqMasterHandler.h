#ifndef __CWX_MQ_MASTER_HANDLER_H__
#define __CWX_MQ_MASTER_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxMqMacro.h"
#include "CwxPackageReader.h"
#include "CwxPackageWriter.h"
#include "CwxMsgBlock.h"

class CwxMqApp;

///slave��master����binlog�Ĵ���handle
class CwxMqMasterHandler : public CwxCmdOp
{
public:
    enum
    {
        RECONN_MASTER_DELAY_SECOND = 4
    };
public:
    ///���캯��
    CwxMqMasterHandler(CwxMqApp* pApp):m_pApp(pApp)
    {
        m_uiConnId = 0;
    }
    ///��������
    virtual ~CwxMqMasterHandler()
    {

    }
public:
    ///���ӽ�������Ҫ��master����sid
    virtual int onConnCreated(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///master�����ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��������master����Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
public:
    CWX_UINT32 getMasterConnId() const
    {
        return m_uiConnId;
    }
private:
    //0���ɹ���-1��ʧ��
    int saveBinlog(CwxMqTss* pTss, char const* szBinLog, CWX_UINT32 uiLen, CWX_UINT64& ullSid);
private:
    CwxMqApp*     m_pApp;  ///<app����
    CWX_UINT32          m_uiConnId; ///<master������ID
    CwxPackageReader      m_reader; 
};

#endif 
