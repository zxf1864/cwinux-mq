#ifndef __CWX_MQ_IMPORT_APP_H__
#define __CWX_MQ_IMPORT_APP_H__
/*
��Ȩ������
    �����Ϊ�������У���ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
�����������⣺
    ��Ѷ��˾������Ѷ��˾��ֱ��ҵ���������ϵ�Ĺ�˾����ʹ�ô������ԭ��ɲο���
http://it.sohu.com/20100903/n274684530.shtml
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxAppFramework.h"
#include "CwxAppHandler4Msg.h"
#include "CwxMqImportConfig.h"
#include "CwxMqTss.h"
#include "CwxMqPoco.h"

CWINUX_USING_NAMESPACE;

///MQ��ѹ������app
class CwxMqImportApp : public CwxAppFramework{
public:
    enum{
        LOG_FILE_SIZE = 30, ///<ÿ��ѭ��������־�ļ���MBTYE
        LOG_FILE_NUM = 7,///<ѭ����־�ļ�������
        SVR_TYPE_ECHO = CwxAppFramework::SVR_TYPE_USER_START ///<echo��ѯ��svr-id����
    };

    ///���캯��
	CwxMqImportApp();
    ///��������
	virtual ~CwxMqImportApp();
    //��ʼ��app, -1:failure, 0 success;
    virtual int init(int argc, char** argv);
public:
    //ʱ����Ӧ����
    virtual void onTime(CwxTimeValue const& current);
    //�ź���Ӧ����
    virtual void onSignal(int signum);
    //echo���ӽ�������
    virtual int onConnCreated(CwxAppHandler4Msg& conn, bool& bSuspendConn, bool& bSuspendListen);
    //echo���ص���Ӧ����
    virtual int onRecvMsg(CwxMsgBlock* msg, CwxAppHandler4Msg const& conn, CwxMsgHead const& header, bool& bSuspendConn);
    //tss
    virtual CwxAppTss* onTssEnv();
protected:
    //init the Enviroment before run.0:success, -1:failure.
	virtual int initRunEnv();
private:
    //����echo����
    void sendNextMsg(CWX_UINT32 uiSvrId, CWX_UINT32 uiHostId, CWX_UINT32 uiConnId);
private:
    CwxMqImportConfig               m_config; ///<�����ļ�����
    char                           m_szBuf100K[100*1024+1]; ///<���͵�echo����buf������
    CWX_UINT32                     m_uiSendNum;///<����echo���������
    CWX_UINT32                     m_uiRecvNum;///<���յ�echo�ظ�������
};

#endif

