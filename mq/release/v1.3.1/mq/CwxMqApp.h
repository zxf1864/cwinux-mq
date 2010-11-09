#ifndef __CWX_MQ_APP_H__
#define __CWX_MQ_APP_H__
/*
��Ȩ������
    �����Ϊ�������У���ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
�����������⣺
    ��Ѷ��˾������Ѷ��˾��ֱ��ҵ���������ϵ�Ĺ�˾����ʹ�ô������ԭ��ɲο���
http://it.sohu.com/20100903/n274684530.shtml
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxMqMacro.h"
#include "CwxAppFramework.h"
#include "CwxAppAioWindow.h"
#include "CwxBinLogMgr.h"
#include "CwxMqConfig.h"
#include "CwxMqMgrServer.h"
#include "CwxMqTss.h"
#include "CwxMqPoco.h"
#include "CwxMqAsyncHandler.h"
#include "CwxMqRecvHandler.h"
#include "CwxMqMasterHandler.h"
#include "CwxMqFetchHandler.h"
#include "CwxMqSysFile.h"
#include "CwxMqQueueMgr.h"

///Ӧ����Ϣ����
#define CWX_MQ_VERSION "1.3.1"
#define CWX_MQ_MODIFY_DATE "2010-11-07 11:30:00"

///MQ�����app����
class CwxMqApp : public CwxAppFramework
{
public:
    enum
    {
        LOG_FILE_SIZE = 30, ///<ÿ����ѭ��ʹ����־�ļ���MByte
        LOG_FILE_NUM = 7, ///<��ѭ��ʹ����־�ļ�������
        SVR_TYPE_RECV = CwxAppFramework::SVR_TYPE_USER_START, ///<master���յ�svr type
        SVR_TYPE_ASYNC = CwxAppFramework::SVR_TYPE_USER_START + 1, ///<master/slave �첽�ַ���svr type
        SVR_TYPE_MASTER = CwxAppFramework::SVR_TYPE_USER_START + 2, ///<slave ��master�������ݵ�svr type
        SVR_TYPE_FETCH = CwxAppFramework::SVR_TYPE_USER_START + 3
    };
    enum
    {
        MIN_SKIP_SID_COUNT = 50, ///<��������ʱ����С��skip sid����
        MAX_SKIP_SID_COUNT = 10000 ///<��������ʱ������skip sid����
    };
    enum
    {
        MQ_NEW_MSG_EVENT=CwxEventInfo::SYS_EVENT_NUM + 1, ///<binlog�������ݵ��¼�
        MQ_CONTINUE_SEND_EVENT= MQ_NEW_MSG_EVENT + 1 ///<δ��ɷ��͵����ӣ���������
    };
    ///���캯��
	CwxMqApp();
    ///��������
	virtual ~CwxMqApp();
    ///���س�ʼ������
    virtual int init(int argc, char** argv);
public:
    ///ʱ����Ӧ����
    virtual void onTime(CwxTimeValue const& current);
    ///signal��Ӧ����
    virtual void onSignal(int signum);
    ///���ӽ���
    virtual int onConnCreated(CwxAppHandler4Msg& conn,
        bool& bSuspendConn,
        bool& bSuspendListen);
    ///���ӹر�
    virtual int onConnClosed(CwxAppHandler4Msg const& conn);
    ///�յ���Ϣ����Ӧ����
    virtual int onRecvMsg(CwxMsgBlock* msg,
                        CwxAppHandler4Msg const& conn,
                        CwxMsgHead const& header,
                        bool& bSuspendConn);
    ///��Ϣ�������
    virtual CWX_UINT32 onEndSendMsg(CwxMsgBlock*& msg,
        CwxAppHandler4Msg const& conn);
    ///��Ϣ����ʧ��
    virtual void onFailSendMsg(CwxMsgBlock*& msg);
public:
    ///�ַ��ȴ��е�binlog
    void dispathWaitingBinlog(CwxMqTss* pTss);
    ///-1:ʧ�ܣ�0���ɹ�
    int commit_mq(char* szErr2K);
    ///��ȡ�ַ��̳߳�
    CwxAppThreadPool*  getWriteThreadPool() 
    {
        return m_pWriteThreadPool;///<��Ϣ���ܵ��̳߳ض���
    }

    ///�Ƿ��ǵ�һ��binlog
    inline bool isFirstBinLog() const
    {
        return m_bFirstBinLog;
    }
    ///����Ϊ�յ��ĵ�һ��binlog
    inline void clearFirstBinLog()
    {
        m_bFirstBinLog = false;
    }
    ///��ȡ��һ��commit��ʱ��
    inline time_t getLastCommitTime() const
    {
        return m_ttLastCommitTime;
    }
    ///������һ��commit��ʱ��
    inline void setLastCommitTime(time_t ttTime)
    {
        m_ttLastCommitTime = ttTime;
    }
    ///��ȡδcommit��log����
    inline CWX_UINT32 getUnCommitLogNum() const
    {
        return m_uiUnCommitLogNum;
    }
    ///����δcommit��log����
    inline CWX_UINT32 incUnCommitLogNum()
    {
        return ++m_uiUnCommitLogNum;
    }
    ///��δcommit��log��������
    inline void zeroUnCommitLogNum()
    {
        m_uiUnCommitLogNum = 0;
    }
    ///��ȡMQ�ϴ�commit��ʱ��
    inline time_t getMqLastCommitTime() const
    {
        return m_ttMqLastCommitTime;
    }
    ///����MQ�ϴ�commit��ʱ��
    inline void setMqLastCommitTime(time_t ttTime)
    {
        m_ttMqLastCommitTime = ttTime;
    }
    ///��ȡMQδcommit�ķַ���Ϣ������
    inline CWX_UINT32 getMqUncommitNum() const
    {
        return m_uiMqUnCommitLogNum;
    }
    ///����MQδcommit�ķַ���Ϣ������
    inline void setMqUncommitNum(CWX_UINT32 uiNum)
    {
        m_uiMqUnCommitLogNum = uiNum;
    }
    ///MQδcommit�ķַ���Ϣ������1
    inline void incMqUncommitNum()
    {
        m_uiMqUnCommitLogNum ++;
    }

    ///����ǰ��SID��1�����أ�ֻ��master���γ�sid
    inline CWX_UINT64 nextSid()
    {
        return ++m_uiCurSid;
    }
    ///��ȡ��ǰ��sid
    inline CWX_UINT64 getCurSid()
    {
        return m_uiCurSid;
    }
    ///��ȡ������Ϣ����
    inline CwxMqConfig const& getConfig() const
    {
        return m_config;
    }
    ///��ȡbinlog manager ����ָ��
    inline CwxBinLogMgr* getBinLogMgr()
    {
        return m_pBinLogMgr;
    }
    ///��ȡmq���й�����
    inline CwxMqQueueMgr* getQueueMgr()
    {
        return m_queueMgr;
    }
    ///��ȡ�첽binlog�ַ���handler����
    inline CwxMqAsyncHandler* getAsyncHandler()
    {
        return m_pAsyncHandler;
    }
    ///��ȡslave��masterͬ��binlog��handler����
    inline CwxMqMasterHandler* getMasterHandler()
    {
        return m_pMasterHandler;
    }
    ///��ȡmaster����binlog��handler����
    inline CwxMqRecvHandler* getRecvHandler()
    {
        return m_pRecvHandler;
    }
    ///��ȡmq fetch handler����
    inline CwxMqFetchHandler* getMqFetchHandler()
    {
        return m_pFetchHandler;
    }
    ///��ȡϵͳ�ļ�����
    inline CwxMqSysFile* getSysFile()
    {
        return m_sysFile;
    }
    ///���·���״̬
    inline void updateAppRunState()
    {
        bool bValid = true;
        char const* szReason = "";
        do
        {
            if (m_pBinLogMgr->isInvalid())
            {
                bValid = false;
                szReason = m_pBinLogMgr->getInvalidMsg();
                break;
            }
            else if (m_pMasterHandler)
            {
                if (!m_pMasterHandler->getMasterConnId())
                {
                    bValid = false;
                    szReason = "Can't connect to master";
                }
            }
            else if (m_sysFile)
            {
                if (!m_sysFile->isValid())
                {
                    bValid = false;
                    szReason = m_sysFile->getErrMsg();
                }

            }

        }while(0);
        setAppRunValid(bValid);
        setAppRunFailReason(szReason);
    }
protected:
    ///�������л�������API
    virtual int initRunEnv();
    virtual void destroy();
private:
    ///����binlog��������-1��ʧ�ܣ�0���ɹ�
    int startBinLogMgr();
    int startNetwork();
private:
    bool                        m_bFirstBinLog; ///<�����������յ��ĵ�һ��binglog
    time_t                      m_ttLastCommitTime; ///<��һ��commit��ʱ��
    CWX_UINT32                  m_uiUnCommitLogNum; ///<����һ��commit������δcommit��binlog����
    time_t                      m_ttMqLastCommitTime; ///<��Ϣ�ַ�sys�ļ��ϴ�commit��ʱ��
    CWX_UINT32                  m_uiMqUnCommitLogNum; ///<��Ϣ�ַ�sys�ļ�δcommit������
    CWX_UINT64                  m_uiCurSid; ///<��ǰ��sid
    CwxMqConfig                 m_config; ///<�����ļ�
    CwxBinLogMgr*                m_pBinLogMgr; ///<binlog�Ĺ������
    CwxMqAsyncHandler*      m_pAsyncHandler; ///<�첽�ַ�handle
    CwxMqMasterHandler*     m_pMasterHandler; ///<��master������Ϣ��handle
    CwxMqRecvHandler*       m_pRecvHandler; ///<����binlog��handle��
    CwxMqFetchHandler*      m_pFetchHandler; ///<mq��ȡ��handle
    CwxMqSysFile*           m_sysFile; ///<mq�ַ��ķַ����¼�ļ�
    CwxMqQueueMgr*          m_queueMgr; ///<���й�����
    CwxAppThreadPool*       m_pWriteThreadPool;///<��Ϣ���ܵ��̳߳ض���
//    CwxAppThreadPool*       m_pDispatchThreadPool; ///<��Ϣ�ַ����̳߳ض���
};
#endif

