#ifndef __CWX_MQ_FETCH_APP_H__
#define __CWX_MQ_FETCH_APP_H__
/*
 版权声明：
 本软件遵循GNU GPL V3（http://www.gnu.org/licenses/gpl.html），
 联系方式：email:cwinux@gmail.com；微博:http://t.sina.com.cn/cwinux
 */
#include "CwxAppFramework.h"
#include "CwxAppHandler4Msg.h"
#include "CwxMqFetchConfig.h"
#include "CwxMqTss.h"
#include "CwxMqPoco.h"

CWINUX_USING_NAMESPACE;

///echo的压力测试app
class CwxMqFetchApp : public CwxAppFramework {
  public:
    enum {
      LOG_FILE_SIZE = 30, ///<每个循环运行日志文件的MBTYE
      LOG_FILE_NUM = 7, ///<循环日志文件的数量
      SVR_TYPE_ECHO = CwxAppFramework::SVR_TYPE_USER_START ///<echo查询的svr-id类型
    };
    ///构造函数
    CwxMqFetchApp();
    ///析构函数
    virtual ~CwxMqFetchApp();
    //初始化app, -1:failure, 0 success;
    virtual int init(int argc, char** argv);
  public:
    //时钟响应函数
    virtual void onTime(CwxTimeValue const& current);
    //信号响应函数
    virtual void onSignal(int signum);
    //echo连接建立函数
    virtual int onConnCreated(CwxAppHandler4Msg& conn, bool& bSuspendConn,
        bool& bSuspendListen);
    //echo返回的响应函数
    virtual int onRecvMsg(CwxMsgBlock* msg, CwxAppHandler4Msg& conn,
        CwxMsgHead const& header, bool& bSuspendConn);
    //tss
    virtual CwxTss* onTssEnv();
  protected:
    //init the Enviroment before run.0:success, -1:failure.
    virtual int initRunEnv();
  private:
    //发送echo请求
    void sendNextMsg(CWX_UINT32 uiSvrId, CWX_UINT32 uiHostId,
        CWX_UINT32 uiConnId);
    ///设置连接的属性
    static int setSockAttr(CWX_HANDLE handle, void* arg);
  private:
    CwxMqFetchConfig m_config; ///<配置文件对象
    char m_szBuf100K[100 * 1024 + 1]; ///<发送的echo数据buf及内容
    CWX_UINT32 m_uiSendNum; ///<发送echo请求的数量
    CWX_UINT32 m_uiRecvNum; ///<接收到echo回复的数量
    CwxPackageWriter* m_pWriter;
};

#endif

