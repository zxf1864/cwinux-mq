#ifndef __CWX_MQ_APP_H__
#define __CWX_MQ_APP_H__
/*
版权声明：
    本软件遵循GNU GPL V3（http://www.gnu.org/licenses/gpl.html），
    联系方式：email:cwinux@gmail.com；微博:http://t.sina.com.cn/cwinux
*/
#include "CwxMqMacro.h"
#include "CwxAppFramework.h"
#include "CwxAppAioWindow.h"
#include "CwxBinLogMgr.h"
#include "CwxMqConfig.h"
#include "CwxMqTss.h"
#include "CwxMqPoco.h"
#include "CwxMqBinAsyncHandler.h"
#include "CwxMqBinRecvHandler.h"
#include "CwxMqMasterHandler.h"
#include "CwxMqBinFetchHandler.h"
#include "CwxMqQueueMgr.h"
#include "CwxThreadPool.h"

///应用信息定义
#define CWX_MQ_VERSION "2.2.10"
#define CWX_MQ_MODIFY_DATE "20110901102000"

///MQ服务的app对象
class CwxMqApp : public CwxAppFramework
{
public:
    enum
    {
        MAX_MONITOR_REPLY_SIZE = 1024 * 1024,
        LOG_FILE_SIZE = 30, ///<每个可循环使用日志文件的MByte
        LOG_FILE_NUM = 7, ///<可循环使用日志文件的数量
        SVR_TYPE_RECV = CwxAppFramework::SVR_TYPE_USER_START, ///<master bin协议接收的svr type
        SVR_TYPE_ASYNC = CwxAppFramework::SVR_TYPE_USER_START + 2, ///<master/slave bin协议异步分发的svr type
        SVR_TYPE_MASTER = CwxAppFramework::SVR_TYPE_USER_START + 4, ///<slave 从master接收数据的svr type
        SVR_TYPE_FETCH = CwxAppFramework::SVR_TYPE_USER_START + 5, ///<mq bin协议消息获取服务类型
        SVR_TYPE_MONITOR = CwxAppFramework::SVR_TYPE_USER_START + 7 ///<监控监听的服务类型
    };
    enum
    {
        MQ_NEW_MSG_EVENT=CwxEventInfo::SYS_EVENT_NUM + 1, ///<binlog有新数据的事件
        MQ_CONTINUE_SEND_EVENT= MQ_NEW_MSG_EVENT + 1 ///<未完成发送的连接，继续发送
    };
    ///构造函数
	CwxMqApp();
    ///析构函数
	virtual ~CwxMqApp();
    ///重载初始化函数
    virtual int init(int argc, char** argv);
public:
    ///时钟响应函数
    virtual void onTime(CwxTimeValue const& current);
    ///signal响应函数
    virtual void onSignal(int signum);
    ///连接建立
    virtual int onConnCreated(CWX_UINT32 uiSvrId,
        CWX_UINT32 uiHostId,
        CWX_HANDLE handle,
        bool& bSuspendListen);
    ///连接建立
    virtual int onConnCreated(CwxAppHandler4Msg& conn,
        bool& bSuspendConn,
        bool& bSuspendListen);
    ///连接关闭
    virtual int onConnClosed(CwxAppHandler4Msg& conn);
    ///收到消息的响应函数
    virtual int onRecvMsg(CwxMsgBlock* msg,
                        CwxAppHandler4Msg& conn,
                        CwxMsgHead const& header,
                        bool& bSuspendConn);
    ///收到消息的响应函数
    virtual int onRecvMsg(CwxAppHandler4Msg& conn,
           bool& bSuspendConn);
public:
    ///-1:失败；0：成功
    int commit_mq();
    ///是否是第一条binlog
    inline bool isFirstBinLog() const
    {
        return m_bFirstBinLog;
    }
    ///设置为收到的第一条binlog
    inline void clearFirstBinLog()
    {
        m_bFirstBinLog = false;
    }
    ///获取上一次commit的时间
    inline CWX_UINT32 getLastCommitTime() const
    {
        return m_ttLastCommitTime;
    }

    ///设置上一次commit的时间
    inline void setLastCommitTime(CWX_UINT32 ttTime)
    {
        m_ttLastCommitTime = ttTime;
    }

    ///获取未commit的log数量
    inline CWX_UINT32 getUnCommitLogNum() const
    {
        return m_uiUnCommitLogNum;
    }

    ///增加未commit的log数量
    inline CWX_UINT32 incUnCommitLogNum()
    {
        return ++m_uiUnCommitLogNum;
    }

    ///将未commit的log数量归零
    inline void zeroUnCommitLogNum()
    {
        m_uiUnCommitLogNum = 0;
    }

    ///获取MQ上次commit的时间
    inline CWX_UINT32 getMqLastCommitTime() const
    {
        return m_ttMqLastCommitTime;
    }

    ///设置MQ上次commit的时间
    inline void setMqLastCommitTime(CWX_UINT32 ttTime)
    {
        m_ttMqLastCommitTime = ttTime;
    }


    ///将当前的SID加1并返回，只有master才形成sid
    inline CWX_UINT64 nextSid()
    {
        return ++m_uiCurSid;
    }

    ///获取当前的sid
    inline CWX_UINT64 getCurSid()
    {
        return m_uiCurSid;
    }

    ///获取配置信息对象
    inline CwxMqConfig const& getConfig() const
    {
        return m_config;
    }

    ///获取binlog manager 对象指针
    inline CwxBinLogMgr* getBinLogMgr()
    {
        return m_pBinLogMgr;
    }

    ///获取mq队列管理器
    inline CwxMqQueueMgr* getQueueMgr()
    {
        return m_queueMgr;
    }

    ///获取slave从master同步binlog的handler对象
    inline CwxMqMasterHandler* getMasterHandler()
    {
        return m_pMasterHandler;
    }

    ///获取master接收binlog的handler对象
    inline CwxMqBinRecvHandler* getBinRecvHandler()
    {
        return m_pBinRecvHandler;
    }

    ///更新服务状态
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
                if (!m_pMasterHandler->isSync())
                {
                    bValid = false;
                    szReason = m_pMasterHandler->getMasterErr().c_str();
                }
            }
            else if (m_queueMgr)
            {
                if (!m_queueMgr->isValid())
                {
                    bValid = false;
                    szReason = m_queueMgr->getErrMsg().c_str();
                }

            }

        }while(0);
        setAppRunValid(bValid);
        setAppRunFailReason(szReason);
    }

    CwxAppChannel* getAsyncDispChannel()
    {
        return m_asyncDispChannel;
    }

    CwxAppChannel* getMqChannel()
    {
        return m_mqChannel;
    }

protected:
    ///重载运行环境设置API
    virtual int initRunEnv();
    virtual void destroy();
private:
    ///启动binlog管理器，-1：失败；0：成功
    int startBinLogMgr();
    int startNetwork();
    ///stats命令，-1：因为错误关闭连接；0：不关闭连接
    int monitorStats(char const* buf, CWX_UINT32 uiDataLen, CwxAppHandler4Msg& conn);
    ///形成监控内容，返回监控内容的长度
    CWX_UINT32 packMonitorInfo();
    ///分发channel的线程函数，arg为app对象
    static void* DispatchThreadMain(CwxTss* tss, CwxMsgQueue* queue, void* arg);
    ///分发channel的队列消息函数。返回值：0：正常；-1：队列停止
    static int DispatchThreadDoQueue(CwxMsgQueue* queue, CwxMqApp* app, CwxAppChannel* channel);
    ///分发mq channel的线程函数，arg为app对象
    static void* MqThreadMain(CwxTss* tss, CwxMsgQueue* queue, void* arg);
    ///分发mq channel的队列消息函数。返回值：0：正常；-1：队列停止
    static int MqThreadDoQueue(CwxMsgQueue* queue, CwxMqApp* app, CwxAppChannel* channel);
    ///设置master recv连接的属性
    static int setMasterRecvSockAttr(CWX_HANDLE handle, void* arg);
    ///设置master dispatch连接的属性
    static int setMasterDispatchSockAttr(CWX_HANDLE handle, void* arg);
    ///设置slave dispatch连接的属性
    static int setSlaveDispatchSockAttr(CWX_HANDLE handle, void* arg);
    ///设置slave report连接的属性
    static int setSlaveReportSockAttr(CWX_HANDLE handle, void* arg);
    ///设置mq连接的属性
    static int setMqSockAttr(CWX_HANDLE handle, void* arg);
private:
    bool                        m_bFirstBinLog; ///<服务启动后，收到的第一条binglog
    CWX_UINT32                  m_ttLastCommitTime; ///<上一次commit的时候
    CWX_UINT32                  m_uiUnCommitLogNum; ///<自上一次commit以来，未commit的binlog数量
    CWX_UINT32                  m_ttMqLastCommitTime; ///<消息分发sys文件上次commit的时候
    CWX_UINT64                  m_uiCurSid; ///<当前的sid
    CwxMqConfig                 m_config; ///<配置文件
    CwxBinLogMgr*               m_pBinLogMgr; ///<binlog的管理对象
    CwxMqMasterHandler*         m_pMasterHandler; ///<从master接收消息的handle
    CwxMqBinRecvHandler*        m_pBinRecvHandler; ///<bin协议接收binlog的handle。
    CwxMqQueueMgr*              m_queueMgr; ///<队列管理器
    CwxThreadPool*              m_pRecvThreadPool;///<消息接受的线程池对象
    CwxThreadPool*              m_pAsyncDispThreadPool; ///<消息异步分发的线程池对象
    CwxAppChannel*              m_asyncDispChannel; ///<消息异步分发的channel
    CwxThreadPool*              m_pMqThreadPool;       ///<mq获取的线程池对象
    CwxAppChannel*              m_mqChannel;           ///<mq获取的channel
    string                      m_strStartTime; ///<启动时间
    char                        m_szBuf[MAX_MONITOR_REPLY_SIZE];///<监控消息的回复buf

};
#endif

