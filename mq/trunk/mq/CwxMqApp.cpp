#include "CwxMqApp.h"
#include "CwxDate.h"
#include "linux/tcp.h"
///���캯��
CwxMqApp::CwxMqApp()
{
    m_bFirstBinLog =  true;
    m_ttLastCommitTime = 0;
    m_uiUnCommitLogNum = 0;
    m_ttMqLastCommitTime = 0;
    m_uiMqUnCommitLogNum = 0;
    m_uiCurSid = 0;
    m_pBinLogMgr = NULL;
    m_pMasterHandler = NULL;
    m_pBinRecvHandler = NULL;
    m_pMcRecvHandler = NULL;
    m_sysFile = NULL;
    m_queueMgr = NULL;
    m_pRecvThreadPool = NULL;
    m_pAsyncDispThreadPool = NULL;
    m_asyncDispChannel = NULL;
    m_pMqThreadPool = NULL;
    m_mqChannel = NULL;
    memset(m_szBuf, 0x00, MAX_MONITOR_REPLY_SIZE);
}

///��������
CwxMqApp::~CwxMqApp()
{
}

///��ʼ��
int CwxMqApp::init(int argc, char** argv)
{
    string strErrMsg;
    ///���ȵ��üܹ���init api
    if (CwxAppFramework::init(argc, argv) == -1) return -1;
    ///����Ƿ�ͨ��-fָ���������ļ�����û�У������Ĭ�ϵ������ļ�
    if ((NULL == this->getConfFile()) || (strlen(this->getConfFile()) == 0))
    {
        this->setConfFile("svr_conf.xml");
    }
    ///���������ļ�����ʧ�����˳�
    if (0 != m_config.loadConfig(getConfFile()))
    {
        CWX_ERROR((m_config.getErrMsg()));
        return -1;
    }
    ///����������־�����level
    setLogLevel(CwxLogger::LEVEL_DEBUG|CwxLogger::LEVEL_ERROR|CwxLogger::LEVEL_INFO|CwxLogger::LEVEL_WARNING);
    return 0;
}

///�������л�����Ϣ
int CwxMqApp::initRunEnv()
{
    ///����ϵͳ��ʱ�Ӽ������С�̶�Ϊ1ms����Ϊ1s��
    this->setClick(1000);//1s
    ///���ù���Ŀ¼
    this->setWorkDir(m_config.getCommon().m_strWorkDir.c_str());
    ///����ѭ��������־������
    this->setLogFileNum(LOG_FILE_NUM);
    ///����ÿ����־�ļ��Ĵ�С
    this->setLogFileSize(LOG_FILE_SIZE*1024*1024);
    ///���üܹ���initRunEnv��ʹ�������õĲ�����Ч
    if (CwxAppFramework::initRunEnv() == -1 ) return -1;
    ///�����ص������ļ���Ϣ�������־�ļ��У��Թ��鿴���
    m_config.outputConfig();
    ///block����signal
    this->blockSignal(SIGTERM);
    this->blockSignal(SIGUSR1);
    this->blockSignal(SIGUSR2);
    this->blockSignal(SIGCHLD);
    this->blockSignal(SIGCLD);
    this->blockSignal(SIGHUP);
    this->blockSignal(SIGPIPE);
    this->blockSignal(SIGALRM);
    this->blockSignal(SIGCONT);
    this->blockSignal(SIGSTOP);
    this->blockSignal(SIGTSTP);
    this->blockSignal(SIGTTOU);

    //set version
    this->setAppVersion(CWX_MQ_VERSION);
    //set last modify date
    this->setLastModifyDatetime(CWX_MQ_MODIFY_DATE);
    //set compile date
    this->setLastCompileDatetime(CWX_COMPILE_DATE(_BUILD_DATE));
    ///���÷���״̬
    this->setAppRunValid(false);
    ///���÷�����Ϣ
    this->setAppRunFailReason("Starting..............");
    ///��������ʱ��
    CwxDate::getDateY4MDHMS2(time(NULL), m_strStartTime);

    //����binlog������
    if (0 != startBinLogMgr()) return -1;
    CwxMqPoco::init(NULL);
    
    if (m_config.getCommon().m_bMaster)
    {
        ///ע�����ݽ���handler
        if (m_config.getMaster().m_recv_bin.getHostName().length())
        {
            m_pBinRecvHandler = new CwxMqBinRecvHandler(this);
            getCommander().regHandle(SVR_TYPE_RECV_BIN, m_pBinRecvHandler);
        }
        if (m_config.getMaster().m_recv_mc.getHostName().length())
        {
            m_pMcRecvHandler = new CwxMqMcRecvHandler(this);
            getCommander().regHandle(SVR_TYPE_RECV_MC, m_pMcRecvHandler);
        }
    }else{
        ///ע��slave��master���ݽ���handler
        m_pMasterHandler = new CwxMqMasterHandler(this);
        getCommander().regHandle(SVR_TYPE_MASTER_BIN, m_pMasterHandler);
    }
    ///�����������������
    if (0 != startNetwork()) return -1;
    ///����recv�̳߳ض��󣬴��̳߳����̵߳�group-idΪTHREAD_GROUP_USER_START��
    ///�̳߳ص��߳�����Ϊ1��
    m_pRecvThreadPool = new CwxThreadPool(CwxAppFramework::THREAD_GROUP_USER_START,
        1,
        getThreadPoolMgr(),
        &getCommander());
    ///�����̵߳�tss����
    CwxTss** pTss = new CwxTss*[1];
    pTss[0] = new CwxMqTss();
    ((CwxMqTss*)pTss[0])->init();
    ///�����߳�
    if ( 0 != m_pRecvThreadPool->start(pTss))
    {
        CWX_ERROR(("Failure to start recv thread pool"));
        return -1;
    }
    //�����ַ��̳߳�
    if (m_config.getCommon().m_bMaster)
    {
        if (m_config.getMaster().m_async_bin.getHostName().length() ||
            m_config.getMaster().m_async_mc.getHostName().length())
        {
            m_asyncDispChannel = new CwxAppChannel();
            m_pAsyncDispThreadPool = new CwxThreadPool( CwxAppFramework::THREAD_GROUP_USER_START + 1,
                1,
                getThreadPoolMgr(),
                &getCommander(),
                CwxMqApp::DispatchThreadMain,
                this);
            ///�����߳�
            pTss = new CwxTss*[1];
            pTss[0] = new CwxMqTss();
            ((CwxMqTss*)pTss[0])->init();
            if ( 0 != m_pAsyncDispThreadPool->start(pTss)){
                CWX_ERROR(("Failure to start dispatch thread pool"));
                return -1;
            }
        }
    }
    else
    {
        if (m_config.getSlave().m_async_bin.getHostName().length() ||
            m_config.getSlave().m_async_mc.getHostName().length())
        {
            m_mqChannel = new CwxAppChannel();
            m_pMqThreadPool = new CwxThreadPool( CwxAppFramework::THREAD_GROUP_USER_START + 2,
                1,
                getThreadPoolMgr(),
                &getCommander(),
                CwxMqApp::MqThreadMain,
                this);
            ///�����߳�
            pTss = new CwxTss*[1];
            pTss[0] = new CwxMqTss();
            ((CwxMqTss*)pTss[0])->init();
            if ( 0 != m_pMqThreadPool->start(pTss)){
                CWX_ERROR(("Failure to start mq thread pool"));
                return -1;
            }
        }
    }
    //����mq�̳߳�
    updateAppRunState();
    return 0;
}

///ʱ�Ӻ���
void CwxMqApp::onTime(CwxTimeValue const& current)
{
    ///���û����onTime����
    CwxAppFramework::onTime(current);
    ///��鳬ʱ
    static time_t ttLastTime = time(NULL);
    if (m_config.getCommon().m_bMaster && (time(NULL) >= ttLastTime + 1))
    {
        ttLastTime = time(NULL);
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(SVR_TYPE_RECV_BIN);
        pBlock->event().setEvent(CwxEventInfo::TIMEOUT_CHECK);
        //����ʱ����¼��������¼�����
        m_pRecvThreadPool->append(pBlock);
    }
}

///�źŴ�����
void CwxMqApp::onSignal(int signum)
{
    switch(signum)
    {
    case SIGQUIT: 
        ///����ؽ���֪ͨ�˳������Ƴ�
        CWX_INFO(("Recv exit signal, exit right now."));
        this->stop();
        break;
    default:
        ///�����źţ�ȫ������
        CWX_INFO(("Recv signal=%d, ignore it.", signum));
        break;
    }
}

int CwxMqApp::onConnCreated(CWX_UINT32 uiSvrId,
                          CWX_UINT32 uiHostId,
                          CWX_HANDLE handle,
                          bool& )
{
    CwxMsgBlock* msg = CwxMsgBlockAlloc::malloc(0);
    msg->event().setSvrId(uiSvrId);
    msg->event().setHostId(uiHostId);
    msg->event().setConnId(CWX_APP_INVALID_CONN_ID);
    msg->event().setIoHandle(handle);
    msg->event().setEvent(CwxEventInfo::CONN_CREATED);
    ///����Ϣ�ŵ��̳߳ض����У����ڲ����̵߳����䴦��handle������
    if ((SVR_TYPE_ASYNC_BIN == uiSvrId) || (SVR_TYPE_ASYNC_MC == uiSvrId))
    {
        m_pAsyncDispThreadPool->append(msg);
    }
    else if ((SVR_TYPE_FETCH_BIN == uiSvrId) || (SVR_TYPE_FETCH_MC == uiSvrId))
    {
        m_pMqThreadPool->append(msg);
    }
    else
    {
        CWX_ASSERT(SVR_TYPE_MONITOR == uiSvrId);
    }
    return 0;
}

///���ӽ���
int CwxMqApp::onConnCreated(CwxAppHandler4Msg& conn, bool& , bool& )
{
    if ((SVR_TYPE_RECV_BIN == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_RECV_MC == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER_BIN == conn.getConnInfo().getSvrId()))
    {
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(conn.getConnInfo().getSvrId());
        pBlock->event().setHostId(conn.getConnInfo().getHostId());
        pBlock->event().setConnId(conn.getConnInfo().getConnId());
        ///�����¼�����
        pBlock->event().setEvent(CwxEventInfo::CONN_CREATED);
        ///���¼���ӵ���Ϣ����
        m_pRecvThreadPool->append(pBlock);
    }
    else if (SVR_TYPE_MONITOR != conn.getConnInfo().getSvrId())
    {///����Ǽ�ص����ӽ���������һ��string��buf�����ڻ��治����������
        string* buf = new string();
        conn.getConnInfo().setUserData(buf);
    }
    else
    {
        CWX_ASSERT(0);
    }
    return 0;
}

///���ӹر�
int CwxMqApp::onConnClosed(CwxAppHandler4Msg& conn)
{
    if ((SVR_TYPE_RECV_BIN == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_RECV_MC == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER_BIN == conn.getConnInfo().getSvrId()))
    {
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(conn.getConnInfo().getSvrId());
        pBlock->event().setHostId(conn.getConnInfo().getHostId());
        pBlock->event().setConnId(conn.getConnInfo().getConnId());
        ///�����¼�����
        pBlock->event().setEvent(CwxEventInfo::CONN_CLOSED);
        m_pRecvThreadPool->append(pBlock);
    }
    else if (SVR_TYPE_MONITOR != conn.getConnInfo().getSvrId())
    {///���Ǽ�ص����ӹرգ�������ͷ���ǰ��������string����
        if (conn.getConnInfo().getUserData())
        {
            delete (string*)conn.getConnInfo().getUserData();
            conn.getConnInfo().setUserData(NULL);
        }
    }
    else
    {
        CWX_ASSERT(0);
    }
    return 0;
}

///�յ���Ϣ
int CwxMqApp::onRecvMsg(CwxMsgBlock* msg,
                        CwxAppHandler4Msg& conn,
                        CwxMsgHead const& header,
                        bool& )
{
    if ((SVR_TYPE_RECV_BIN == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER_BIN == conn.getConnInfo().getSvrId()))
    {
        msg->event().setSvrId(conn.getConnInfo().getSvrId());
        msg->event().setHostId(conn.getConnInfo().getHostId());
        msg->event().setConnId(conn.getConnInfo().getConnId());
        ///������Ϣͷ
        msg->event().setMsgHeader(header);
        ///�����¼�����
        msg->event().setEvent(CwxEventInfo::RECV_MSG);
        ///����Ϣ�ŵ��̳߳ض����У����ڲ����̵߳����䴦��handle������
        m_pRecvThreadPool->append(msg);
        return 0;
    }
    else
    {
        CWX_ASSERT(0);
    }
    return 0;
}

///�յ���Ϣ����Ӧ����
int CwxMqApp::onRecvMsg(CwxAppHandler4Msg& conn,
                      bool& )
{
    if(SVR_TYPE_RECV_MC == conn.getConnInfo().getSvrId())
    {
        return 0;
    }else if (SVR_TYPE_MONITOR == conn.getConnInfo().getSvrId())
    {
        char  szBuf[1024];
        ssize_t recv_size = CwxSocket::recv(conn.getHandle(),
            szBuf,
            1024);
        if (recv_size <=0 )
        { //error or signal
            if ((0==recv_size) || ((errno != EWOULDBLOCK) && (errno != EINTR)))
            {
                return -1; //error
            }
            else
            {//signal or no data
                return 0;
            }
        }
        ///�����Ϣ
        return monitorStats(szBuf, (CWX_UINT32)recv_size, conn);
    }
    else
    {
        CWX_ASSERT(0);
    }
    return -1;
}


void CwxMqApp::destroy()
{
    if (m_pRecvThreadPool)
    {
        m_pRecvThreadPool->stop();
        delete m_pRecvThreadPool;
        m_pRecvThreadPool = NULL;
    }
    if (m_pAsyncDispThreadPool)
    {
        m_pAsyncDispThreadPool->stop();
        delete m_pAsyncDispThreadPool;
        m_pAsyncDispThreadPool = NULL;
    }
    if (m_asyncDispChannel)
    {
        delete m_asyncDispChannel;
        m_asyncDispChannel = NULL;
    }
    if (m_pMqThreadPool)
    {
        m_pMqThreadPool->stop();
        delete m_pMqThreadPool;
        m_pMqThreadPool = NULL;
    }
    if (m_mqChannel)
    {
        delete m_mqChannel;
        m_mqChannel = NULL;
    }
    if (m_queueMgr)
    {
        delete m_queueMgr;
        m_queueMgr = NULL;
    }
    if (m_pMasterHandler)
    {
        delete m_pMasterHandler;
        m_pMasterHandler = NULL;
    }
    if (m_pBinRecvHandler)
    {
        delete m_pBinRecvHandler;
        m_pBinRecvHandler = NULL;
    }
    if (m_pMcRecvHandler)
    {
        delete m_pMcRecvHandler;
        m_pMcRecvHandler = NULL;
    }
    if (m_pBinLogMgr)
    {
        m_pBinLogMgr->commit();
        delete m_pBinLogMgr;
        m_pBinLogMgr = NULL;
    }
    if (m_sysFile)
    {
        m_sysFile->commit();
        delete m_sysFile;
        m_sysFile = NULL;
    }
    CwxMqPoco::destory();

    CwxAppFramework::destroy();
}


///����binlog��������-1��ʧ�ܣ�0���ɹ�
int CwxMqApp::startBinLogMgr()
{
    ///��ʼ��binlog
    {
        CWX_UINT64 ullBinLogSize = m_config.getBinLog().m_uiBinLogMSize;
        ullBinLogSize *= 1024 * 1024;
        m_pBinLogMgr = new CwxBinLogMgr(m_config.getBinLog().m_strBinlogPath.c_str(),
            m_config.getBinLog().m_strBinlogPrex.c_str(),
            ullBinLogSize,
            m_config.getBinLog().m_bDelOutdayLogFile);
        if (0 != m_pBinLogMgr->init(m_config.getBinLog().m_uiMgrMaxDay,
            CWX_TSS_2K_BUF))
        {///<���ʧ�ܣ��򷵻�-1
            CWX_ERROR(("Failure to init binlog manager, error:%s", CWX_TSS_2K_BUF));
            return -1;
        }
        m_bFirstBinLog = true;
        m_ttLastCommitTime = time(NULL);
        m_uiUnCommitLogNum = 0;
        ///��ȡsid
        if (m_config.getBinLog().m_uiFlushNum > MAX_SKIP_SID_COUNT)
        {
            m_uiCurSid = m_pBinLogMgr->getMaxSid() + MAX_SKIP_SID_COUNT;
        }
        else if (m_config.getBinLog().m_uiFlushNum < MIN_SKIP_SID_COUNT)
        {
            m_uiCurSid = m_pBinLogMgr->getMaxSid() + MIN_SKIP_SID_COUNT;
        }
        else
        {
            m_uiCurSid = m_pBinLogMgr->getMaxSid() + m_config.getBinLog().m_uiFlushNum;
        }
    }
    //��ʼ��MQ
    if (m_config.getMq().m_queues.size())
    {
        ///��ʼ��mq�ַ���ϵͳ�ļ�
        {
            m_ttMqLastCommitTime = time(NULL);
            m_uiMqUnCommitLogNum = 0;
            if (m_sysFile) delete m_sysFile;
            set<string> queues;
            map<string, CwxMqConfigQueue>::const_iterator iter =  m_config.getMq().m_queues.begin();
            while(iter != m_config.getMq().m_queues.end())
            {
                queues.insert(iter->first);
                iter ++;
            }
            m_sysFile = new CwxMqSysFile(100, m_config.getBinLog().m_strBinlogPath + ".cwx_mq_fetch_pos.dat");
            if (0 != m_sysFile->init(queues))
            {
                CWX_ERROR(("Failure to init sys file, err:%s", m_sysFile->getErrMsg()));
                return -1;
            }
        }
        //��ʼ�����й�����
        if (m_queueMgr) delete m_queueMgr;
        m_queueMgr = new CwxMqQueueMgr();
        if (0 != m_queueMgr->init(m_pBinLogMgr,
            m_sysFile->getQueues(),
            m_config.getMq().m_queues))
        {
            CWX_ERROR(("Failure to init mq queue manager"));
            return -1;
        }
    }
    ///�ɹ�
    return 0;
}

int CwxMqApp::startNetwork()
{
    ///�򿪼����ķ������˿ں�
    if (m_config.getCommon().m_monitor.getHostName().length())
    {
        if (0 > this->noticeTcpListen(SVR_TYPE_MONITOR,
            m_config.getCommon().m_monitor.getHostName().c_str(),
            m_config.getCommon().m_monitor.getPort(),
            true))
        {
            CWX_ERROR(("Can't register the monitor tcp accept listen: addr=%s, port=%d",
                m_config.getCommon().m_monitor.getHostName().c_str(),
                m_config.getCommon().m_monitor.getPort()));
            return -1;
        }
    }
    ///������
    if (m_config.getCommon().m_bMaster)
    {
        ///��binЭ����Ϣ���յ�listen
        if (m_config.getMaster().m_recv_bin.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_RECV_BIN, 
                m_config.getMaster().m_recv_bin.getHostName().c_str(),
                m_config.getMaster().m_recv_bin.getPort(),
                false,
                m_config.getMaster().m_recv_bin.isKeepAlive()))
            {
                CWX_ERROR(("Can't register the recv tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_recv_bin.getHostName().c_str(),
                    m_config.getMaster().m_recv_bin.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_recv_bin.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_RECV_BIN,
                m_config.getMaster().m_recv_bin.getUnixDomain().c_str(),
                false,
                m_config.getMaster().m_recv_bin.isKeepAlive()))
            {
                CWX_ERROR(("Can't register the recv unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_recv_bin.getUnixDomain().c_str()));
                return -1;
            }
        }

        ///��mcЭ����Ϣ���յ�listen
        if (m_config.getMaster().m_recv_mc.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_RECV_MC, 
                m_config.getMaster().m_recv_mc.getHostName().c_str(),
                m_config.getMaster().m_recv_mc.getPort(),
                true,
                m_config.getMaster().m_recv_mc.isKeepAlive()))
            {
                CWX_ERROR(("Can't register the recv tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_recv_mc.getHostName().c_str(),
                    m_config.getMaster().m_recv_mc.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_recv_mc.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_RECV_MC,
                m_config.getMaster().m_recv_mc.getUnixDomain().c_str(),
                true,
                m_config.getMaster().m_recv_mc.isKeepAlive()))
            {
                CWX_ERROR(("Can't register the recv unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_recv_mc.getUnixDomain().c_str()));
                return -1;
            }
        }

        ///��binЭ���첽�ַ�
        if (m_config.getMaster().m_async_bin.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC_BIN, 
                m_config.getMaster().m_async_bin.getHostName().c_str(),
                m_config.getMaster().m_async_bin.getPort(),
                true,
                m_config.getMaster().m_async_bin.isKeepAlive(),
                CWX_APP_EVENT_MODE,
                m_config.getCommon().m_uiSockBufSize * 1024,
                m_config.getCommon().m_uiSockBufSize * 1024
                ))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_async_bin.getHostName().c_str(),
                    m_config.getMaster().m_async_bin.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_async_bin.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC_BIN,
                m_config.getMaster().m_async_bin.getUnixDomain().c_str(),
                true,
                m_config.getMaster().m_async_bin.isKeepAlive(),
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_async_bin.getUnixDomain().c_str()));
                return -1;
            }
        }

        ///��mcЭ���첽�ַ�
        if (m_config.getMaster().m_async_mc.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC_MC, 
                m_config.getMaster().m_async_mc.getHostName().c_str(),
                m_config.getMaster().m_async_mc.getPort(),
                true,
                m_config.getMaster().m_async_mc.isKeepAlive(),
                CWX_APP_EVENT_MODE,
                m_config.getCommon().m_uiSockBufSize * 1024,
                m_config.getCommon().m_uiSockBufSize * 1024
                ))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_async_mc.getHostName().c_str(),
                    m_config.getMaster().m_async_mc.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_async_mc.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC_MC,
                m_config.getMaster().m_async_mc.getUnixDomain().c_str(),
                true,
                m_config.getMaster().m_async_mc.isKeepAlive(),
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_async_mc.getUnixDomain().c_str()));
                return -1;
            }
        }
    }
    else
    {
        ///����binЭ��master
        if (m_config.getSlave().m_master_bin.getUnixDomain().length())
        {
            if (0 > this->noticeLsockConnect(SVR_TYPE_MASTER_BIN, 0, 
                m_config.getSlave().m_master_bin.getUnixDomain().c_str(),
                false,
                true,
                1,
                2))
            {
                CWX_ERROR(("Can't register the master unix-domain connect: path-file=%s",
                    m_config.getSlave().m_master_bin.getUnixDomain().c_str()));
                return -1;
            }
        }
        else if (m_config.getSlave().m_master_bin.getHostName().length())
        {
            if (0 > this->noticeTcpConnect(SVR_TYPE_MASTER_BIN, 0, 
                m_config.getSlave().m_master_bin.getHostName().c_str(),
                m_config.getSlave().m_master_bin.getPort(),
                false,
                m_config.getSlave().m_master_bin.isKeepAlive(),
                1,
                2,
                m_config.getCommon().m_uiSockBufSize * 1024,
                m_config.getCommon().m_uiSockBufSize * 1024))
            {
                CWX_ERROR(("Can't register the master tcp connect: addr=%s, port=%d",
                    m_config.getSlave().m_master_bin.getHostName().c_str(),
                    m_config.getSlave().m_master_bin.getPort()));
                return -1;
            }
        }
        else
        {
            CWX_ERROR(("Not config master dispatch"));
            return -1;
        }
        ///binЭ���첽�ַ�
        if (m_config.getSlave().m_async_bin.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC_BIN, 
                m_config.getSlave().m_async_bin.getHostName().c_str(),
                m_config.getSlave().m_async_bin.getPort(),
                true,
                true,
                CWX_APP_EVENT_MODE,
                m_config.getCommon().m_uiSockBufSize * 1024,
                m_config.getCommon().m_uiSockBufSize * 1024))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getSlave().m_async_bin.getHostName().c_str(),
                    m_config.getSlave().m_async_bin.getPort()));
                return -1;
            }
        }
        if (m_config.getSlave().m_async_bin.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC_BIN,
                m_config.getSlave().m_async_bin.getUnixDomain().c_str(),
                true,
                false,
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getSlave().m_async_bin.getUnixDomain().c_str()));
                return -1;
            }
        }
        ///mcЭ���첽�ַ�
        if (m_config.getSlave().m_async_mc.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC_MC, 
                m_config.getSlave().m_async_mc.getHostName().c_str(),
                m_config.getSlave().m_async_mc.getPort(),
                true,
                true,
                CWX_APP_EVENT_MODE,
                m_config.getCommon().m_uiSockBufSize * 1024,
                m_config.getCommon().m_uiSockBufSize * 1024))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getSlave().m_async_mc.getHostName().c_str(),
                    m_config.getSlave().m_async_mc.getPort()));
                return -1;
            }
        }
        if (m_config.getSlave().m_async_mc.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC_MC,
                m_config.getSlave().m_async_mc.getUnixDomain().c_str(),
                true,
                false,
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getSlave().m_async_mc.getUnixDomain().c_str()));
                return -1;
            }
        }
    }
    //��bin mq��ȡ�ļ����˿�
    if (m_config.getMq().m_binListen.getHostName().length())
    {
        if (0 > this->noticeTcpListen(SVR_TYPE_FETCH_BIN, 
            m_config.getMq().m_binListen.getHostName().c_str(),
            m_config.getMq().m_binListen.getPort(),
            true,
            m_config.getMq().m_binListen.isKeepAlive(),
            CWX_APP_EVENT_MODE,
            m_config.getCommon().m_uiSockBufSize * 1024,
            m_config.getCommon().m_uiSockBufSize * 1024))
        {
            CWX_ERROR(("Can't register the mq-fetch tcp accept listen: addr=%s, port=%d",
                m_config.getMq().m_binListen.getHostName().c_str(),
                m_config.getMq().m_binListen.getPort()));
            return -1;
        }
    }
    if (m_config.getMq().m_binListen.getUnixDomain().length())
    {
        if (0 > this->noticeLsockListen(SVR_TYPE_FETCH_BIN,
            m_config.getMq().m_binListen.getUnixDomain().c_str(),
            true,
            false,
            CWX_APP_EVENT_MODE))
        {
            CWX_ERROR(("Can't register the mq-fetch unix-domain accept listen: path-file=%s",
                m_config.getMq().m_binListen.getUnixDomain().c_str()));
            return -1;
        }
    }
    //��mc mq��ȡ�ļ����˿�
    if (m_config.getMq().m_mcListen.getHostName().length())
    {
        if (0 > this->noticeTcpListen(SVR_TYPE_FETCH_BIN, 
            m_config.getMq().m_mcListen.getHostName().c_str(),
            m_config.getMq().m_mcListen.getPort(),
            true,
            m_config.getMq().m_mcListen.isKeepAlive(),
            CWX_APP_EVENT_MODE,
            m_config.getCommon().m_uiSockBufSize * 1024,
            m_config.getCommon().m_uiSockBufSize * 1024))
        {
            CWX_ERROR(("Can't register the mq-fetch tcp accept listen: addr=%s, port=%d",
                m_config.getMq().m_mcListen.getHostName().c_str(),
                m_config.getMq().m_mcListen.getPort()));
            return -1;
        }
    }
    if (m_config.getMq().m_mcListen.getUnixDomain().length())
    {
        if (0 > this->noticeLsockListen(SVR_TYPE_FETCH_BIN,
            m_config.getMq().m_mcListen.getUnixDomain().c_str(),
            true,
            false,
            CWX_APP_EVENT_MODE))
        {
            CWX_ERROR(("Can't register the mq-fetch unix-domain accept listen: path-file=%s",
                m_config.getMq().m_mcListen.getUnixDomain().c_str()));
            return -1;
        }
    }

    return 0;
}

int CwxMqApp::commit_mq(char* szErr2K)
{
    int iRet = 0;
    CWX_INFO(("Begin flush mq sys file......."));
    if (getMqUncommitNum())
    {
        iRet = getSysFile()->commit();
        if ((0 != iRet) && szErr2K) strcpy(szErr2K, getSysFile()->getErrMsg());
        setMqUncommitNum(0);
        setMqLastCommitTime(time(NULL));
    }
    CWX_INFO(("End flush mq sys file......."));
    return iRet;
}


int CwxMqApp::monitorStats(char const* buf, CWX_UINT32 uiDataLen, CwxAppHandler4Msg& conn)
{
    string* strCmd = (string*)conn.getConnInfo().getUserData();
    strCmd->append(buf, uiDataLen);
    CwxMsgBlock* msg = NULL;
    string::size_type end = 0;
    do
    {
        CwxCommon::trim(*strCmd);
        end = strCmd->find('\n');
        if (string::npos == end)
        {
            if (strCmd->length() > 10)
            {//��Ч������
                strCmd->erase(); ///��ս��ܵ�������
                ///�ظ���Ϣ
                msg = CwxMsgBlockAlloc::malloc(1024);
                strcpy(msg->wr_ptr(), "ERROR\r\n");
                msg->wr_ptr(strlen(msg->wr_ptr()));
                return -1;
            }
            else
            {
                return 0;
            }
        }
        else
        {
            if (memcmp(strCmd->c_str(), "stats", 5) == 0)
            {
                strCmd->erase(); ///��ս��ܵ�������
                CWX_UINT32 uiLen = packMonitorInfo();
                msg = CwxMsgBlockAlloc::malloc(uiLen);
                memcpy(msg->wr_ptr(), m_szBuf, uiLen);
                msg->wr_ptr(uiLen);
            }
            else if(memcmp(strCmd->c_str(), "quit", 4) == 0)
            {
                return -1;
            }  
            else
            {//��Ч������
                strCmd->erase(); ///��ս��ܵ�������
                ///�ظ���Ϣ
                msg = CwxMsgBlockAlloc::malloc(1024);
                strcpy(msg->wr_ptr(), "ERROR\r\n");
                msg->wr_ptr(strlen(msg->wr_ptr()));
            }
        }
    }
    while(0);

    msg->send_ctrl().setConnId(conn.getConnInfo().getConnId());
    msg->send_ctrl().setSvrId(CwxMqApp::SVR_TYPE_MONITOR);
    msg->send_ctrl().setHostId(0);
    msg->send_ctrl().setMsgAttr(CwxMsgSendCtrl::NONE);
    if (-1 == sendMsgByConn(msg))
    {
        CWX_ERROR(("Failure to send monitor reply"));
        CwxMsgBlockAlloc::free(msg);
        return -1;
    }
    return 0;
}

#define MQ_MONITOR_APPEND()\
    uiLen = strlen(szLine);\
    if (uiPos + uiLen > MAX_MONITOR_REPLY_SIZE - 20) break;\
    memcpy(m_szBuf + uiPos, szLine, uiLen);\
    uiPos += uiLen; \

CWX_UINT32 CwxMqApp::packMonitorInfo()
{
    string strValue;
    char szTmp[128];
    char szLine[4096];
    CWX_UINT32 uiLen = 0;
    CWX_UINT32 uiPos = 0;
    do
    {
        //�������pid
        CwxCommon::snprintf(szLine, 4096, "STAT pid %d\r\n", getpid());
        MQ_MONITOR_APPEND();
        //���������pid
        CwxCommon::snprintf(szLine, 4096, "STAT ppid %d\r\n", getppid());
        MQ_MONITOR_APPEND();
        //�汾��
        CwxCommon::snprintf(szLine, 4096, "STAT version %s\r\n", this->getAppVersion().c_str());
        MQ_MONITOR_APPEND();
        //�޸�ʱ��
        CwxCommon::snprintf(szLine, 4096, "STAT modify %s\r\n", this->getLastModifyDatetime().c_str());
        MQ_MONITOR_APPEND();
        //����ʱ��
        CwxCommon::snprintf(szLine, 4096, "STAT compile %s\r\n", this->getLastCompileDatetime().c_str());
        MQ_MONITOR_APPEND();
        //����ʱ��
        CwxCommon::snprintf(szLine, 4096, "STAT start %s\r\n", m_strStartTime.c_str());
        MQ_MONITOR_APPEND();
        //master
        CwxCommon::snprintf(szLine, 4096, "STAT server_type %s\r\n", getConfig().getCommon().m_bMaster?"master":"slave");
        MQ_MONITOR_APPEND();
        //state
        CwxCommon::snprintf(szLine, 4096, "STAT state %s\r\n", isAppRunValid()?"valid":"invalid");
        MQ_MONITOR_APPEND();
        //error
        CwxCommon::snprintf(szLine, 4096, "STAT err '%s'\r\n", isAppRunValid()?"":getAppRunFailReason().c_str());
        MQ_MONITOR_APPEND();
        //binlog
        CwxCommon::snprintf(szLine, 4096, "STAT binlog_state %s\r\n",
            m_pBinLogMgr->isInvalid()?"invalid":"valid");
        MQ_MONITOR_APPEND();
        CwxCommon::snprintf(szLine, 4096, "STAT binlog_error %s\r\n",
            m_pBinLogMgr->isInvalid()? m_pBinLogMgr->getInvalidMsg():"");
        MQ_MONITOR_APPEND();
        CwxCommon::snprintf(szLine, 4096, "STAT min_sid %s\r\n",
            CwxCommon::toString(m_pBinLogMgr->getMinSid(), szTmp));
        MQ_MONITOR_APPEND();
        CwxDate::getDateY4MDHMS2(m_pBinLogMgr->getMinTimestamp(), strValue);
        CwxCommon::snprintf(szLine, 4096, "STAT min_sid_time %s\r\n",
            strValue.c_str());
        MQ_MONITOR_APPEND();
        CwxCommon::snprintf(szLine, 4096, "STAT min_binlog_file %s\r\n",
            m_pBinLogMgr->getMinFile(strValue).c_str());
        MQ_MONITOR_APPEND();
        CwxCommon::snprintf(szLine, 4096, "STAT max_sid %s\r\n",
            CwxCommon::toString(m_pBinLogMgr->getMaxSid(), szTmp));
        MQ_MONITOR_APPEND();
        CwxDate::getDateY4MDHMS2(m_pBinLogMgr->getMaxTimestamp(), strValue);
        CwxCommon::snprintf(szLine, 4096, "STAT max_sid_time %s\r\n",
            strValue.c_str());
        MQ_MONITOR_APPEND();
        CwxCommon::snprintf(szLine, 4096, "STAT max_binlog_file %s\r\n",
            m_pBinLogMgr->getMaxFile(strValue).c_str());
        MQ_MONITOR_APPEND();
        CWX_UINT64 ullMqSid = 0;
        CWX_UINT64 ullMqNum = 0;
        CwxMqQueue* pQueue = 0;
        char szSid1[64];
        char szSid2[64];
        map<string, CwxMqConfigQueue>::const_iterator iter = getConfig().getMq().m_queues.begin();
        while(iter != getConfig().getMq().m_queues.end())
        {
            pQueue = getQueueMgr()->getQueue(iter->first);
            ullMqSid = pQueue->getCurSid(); 
            ullMqNum = pQueue->getMqNum();
            CwxCommon::toString(ullMqSid, szSid1, 10);
            CwxCommon::toString(ullMqNum, szSid2, 10);
            CwxCommon::snprintf(szLine, 4096, "STAT mq name(%s):sid(%s):mq_num(%s)\r\n",
                iter->first.c_str(), szSid1, szSid2);
            MQ_MONITOR_APPEND();
            iter++;
        }
    }
    while(0);
    strcpy(m_szBuf + uiPos, "END\r\n");
    return strlen(m_szBuf);

}

///�ַ�channel���̺߳�����argΪapp����
void* CwxMqApp::DispatchThreadMain(CwxTss* tss, CwxMsgQueue* queue, void* arg)
{
    CwxMqApp* app = (CwxMqApp*) arg;
    if (0 != app->getAsyncDispChannel()->open())
    {
        CWX_ERROR(("Failure to open async dispatch channel"));
        return NULL;
    }
    while(1)
    {
        //��ȡ�����е���Ϣ������
        if (0 != DispatchThreadDoQueue(queue, app, app->getAsyncDispChannel())) break;
        if (-1 == app->getAsyncDispChannel()->dispatch(5))
        {
            CWX_ERROR(("Failure to invoke async dispatch channel CwxAppChannel::dispatch()"));
            sleep(1);
        }
    }
    app->getAsyncDispChannel()->stop();
    app->getAsyncDispChannel()->close();
    if (!app->isStopped())
    {
        CWX_INFO(("Stop app for disptch channel thread stopped."));
        app->stop();
    }
    return NULL;
}
///�ַ�channel�Ķ�����Ϣ����������ֵ��0��������-1������ֹͣ
int CwxMqApp::DispatchThreadDoQueue(CwxMsgQueue* queue, CwxMqApp* app, CwxAppChannel* channel)
{
    int iRet = 0;
    CwxMsgBlock* block = NULL;
    CwxAppHandler4Channel* handler = NULL;
    while (!queue->isEmpty())
    {
        do 
        {
            iRet = queue->dequeue(block);
            if (-1 == iRet) return -1;
            CWX_ASSERT(block->event().getEvent() == CwxEventInfo::CONN_CREATED);
            CWX_ASSERT((block->event().getSvrId() == SVR_TYPE_ASYNC_BIN) ||
                (block->event().getSvrId() == SVR_TYPE_ASYNC_MC));

            if (channel->isRegIoHandle(block->event().getIoHandle()))
            {
                CWX_ERROR(("Handler[%] is register", block->event().getIoHandle()));
                break;
            }
            if (block->event().getSvrId() == SVR_TYPE_ASYNC_BIN)
            {
                handler = new CwxMqBinAsyncHandler(app, channel);
            }
            else if (block->event().getSvrId() == SVR_TYPE_ASYNC_MC)
            {
                handler = new CwxMqMcAsyncHandler(app, channel);
            }
            else
            {
                CWX_ERROR(("Invalid svr_type[%d], close handle[%d]", block->event().getSvrId(), block->event().getIoHandle()));
                ::close(block->event().getIoHandle());
            }
            handler->setHandle(block->event().getIoHandle());
            if (0 != channel->registerHandler(handler->getHandle(),
                handler,
                CwxAppHandler4Base::PREAD_MASK))
            {
                CWX_ERROR(("Failure to register handler[%d]", handler->getHandle()));
                delete handler;
                break;
            }
        } while(0);
        CwxMsgBlockAlloc::free(block);
        block = NULL;
    }
    if (queue->isDeactived()) return -1;
    return 0;
}

///�ַ�mq channel���̺߳�����argΪapp����
void* CwxMqApp::MqThreadMain(CwxTss* tss, CwxMsgQueue* queue, void* arg)
{
    CwxMqApp* app = (CwxMqApp*) arg;
    if (0 != app->getMqChannel()->open())
    {
        CWX_ERROR(("Failure to open mq channel"));
        return NULL;
    }
    while(1)
    {
        //��ȡ�����е���Ϣ������
        if (0 != MqThreadDoQueue(queue, app,  app->getMqChannel())) break;
        if (-1 == app->getMqChannel()->dispatch(5))
        {
            CWX_ERROR(("Failure to invoke mq channel CwxAppChannel::dispatch()"));
            sleep(1);
        }
    }
    app->getMqChannel()->stop();
    app->getMqChannel()->close();
    if (!app->isStopped())
    {
        CWX_INFO(("Stop app for mq channel thread stopped."));
        app->stop();
    }
    return NULL;

}
///�ַ�mq channel�Ķ�����Ϣ����������ֵ��0��������-1������ֹͣ
int CwxMqApp::MqThreadDoQueue(CwxMsgQueue* queue, CwxMqApp* app, CwxAppChannel* channel)
{
    int iRet = 0;
    CwxMsgBlock* block = NULL;
    CwxAppHandler4Channel* handler = NULL;
    while (!queue->isEmpty())
    {
        do 
        {
            iRet = queue->dequeue(block);
            if (-1 == iRet) return -1;
            CWX_ASSERT(block->event().getEvent() == CwxEventInfo::CONN_CREATED);
            CWX_ASSERT((block->event().getSvrId() == SVR_TYPE_FETCH_BIN) ||
                       (block->event().getSvrId() == SVR_TYPE_FETCH_MC));

            if (channel->isRegIoHandle(block->event().getIoHandle()))
            {
                CWX_ERROR(("Handler[%] is register", block->event().getIoHandle()));
                break;
            }
            if (block->event().getSvrId() == SVR_TYPE_FETCH_BIN)
            {
                handler = new CwxMqBinFetchHandler(app, channel);
            }
            else if (block->event().getSvrId() == SVR_TYPE_FETCH_MC)
            {
                handler = new CwxMqMcFetchHandler(app, channel);
            }
            else
            {
                CWX_ERROR(("Invalid svr_type[%d], close handle[%d]", block->event().getSvrId(), block->event().getIoHandle()));
                ::close(block->event().getIoHandle());
            }
            handler->setHandle(block->event().getIoHandle());
            if (0 != channel->registerHandler(handler->getHandle(),
                handler,
                CwxAppHandler4Base::PREAD_MASK))
            {
                CWX_ERROR(("Failure to register handler[%d]", handler->getHandle()));
                delete handler;
                break;
            }
        } while(0);
        CwxMsgBlockAlloc::free(block);
        block = NULL;
    }
    if (queue->isDeactived()) return -1;
    return 0;
}
