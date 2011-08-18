#include "CwxMqApp.h"
#include "CwxDate.h"
///���캯��
CwxMqApp::CwxMqApp()
{
    m_bFirstBinLog =  true;
    m_ttLastCommitTime = 0;
    m_uiUnCommitLogNum = 0;
    m_ttMqLastCommitTime = 0;
    m_uiCurSid = 0;
    m_pBinLogMgr = NULL;
    m_pMasterHandler = NULL;
    m_pBinRecvHandler = NULL;
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
    setLogLevel(CwxLogger::LEVEL_ERROR|CwxLogger::LEVEL_INFO|CwxLogger::LEVEL_WARNING);
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
        if (m_config.getMaster().m_recv.getHostName().length())
        {
            m_pBinRecvHandler = new CwxMqBinRecvHandler(this);
            getCommander().regHandle(SVR_TYPE_RECV, m_pBinRecvHandler);
        }
    }else{
        ///ע��slave��master���ݽ���handler
        m_pMasterHandler = new CwxMqMasterHandler(this);
        getCommander().regHandle(SVR_TYPE_MASTER, m_pMasterHandler);
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
        if (m_config.getMaster().m_async.getHostName().length() ||
            m_config.getMaster().m_async.getUnixDomain().length())
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
        if (m_config.getSlave().m_async.getHostName().length() ||
            m_config.getSlave().m_async.getUnixDomain().length())
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

    //����mq�̳߳�
    if (m_config.getMq().m_mq.getHostName().length() ||
        m_config.getMq().m_mq.getUnixDomain().length())
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
    return 0;
}

///ʱ�Ӻ���
void CwxMqApp::onTime(CwxTimeValue const& current)
{
    ///���û����onTime����
    CwxAppFramework::onTime(current);
    ///��鳬ʱ
    static CWX_UINT32 ttBinLogLastTime = time(NULL);
    static CWX_UINT32 ttMqLogLastTime = time(NULL);
    CWX_UINT32 uiNow = time(NULL);
    if (m_config.getCommon().m_bMaster && (uiNow >= ttBinLogLastTime + 1))
    {
        ttBinLogLastTime = uiNow;
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(SVR_TYPE_RECV);
        pBlock->event().setEvent(CwxEventInfo::TIMEOUT_CHECK);
        //����ʱ����¼��������¼�����
        m_pRecvThreadPool->append(pBlock);
    }
    if (m_queueMgr &&
        (uiNow >= ttMqLogLastTime + 1))
    {
        ttMqLogLastTime = uiNow;
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(SVR_TYPE_FETCH);
        pBlock->event().setEvent(CwxEventInfo::TIMEOUT_CHECK);
        //����ʱ����¼��������¼�����
        m_pMqThreadPool->append(pBlock);
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
    if (SVR_TYPE_ASYNC == uiSvrId)
    {
        if (m_pAsyncDispThreadPool->append(msg) <= 1)
        {
            m_asyncDispChannel->notice();
        }
    }
    else if (SVR_TYPE_FETCH == uiSvrId)
    {
        if (m_pMqThreadPool->append(msg) <= 1)
        {
            m_mqChannel->notice();
        }
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
    if ((SVR_TYPE_RECV == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER == conn.getConnInfo().getSvrId()))
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
    else if (SVR_TYPE_MONITOR == conn.getConnInfo().getSvrId())
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
    if ((SVR_TYPE_RECV == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER == conn.getConnInfo().getSvrId()))
    {
        CwxMsgBlock* pBlock = CwxMsgBlockAlloc::malloc(0);
        pBlock->event().setSvrId(conn.getConnInfo().getSvrId());
        pBlock->event().setHostId(conn.getConnInfo().getHostId());
        pBlock->event().setConnId(conn.getConnInfo().getConnId());
        ///�����¼�����
        pBlock->event().setEvent(CwxEventInfo::CONN_CLOSED);
        m_pRecvThreadPool->append(pBlock);
    }
    else if (SVR_TYPE_MONITOR == conn.getConnInfo().getSvrId())
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
    if ((SVR_TYPE_RECV == conn.getConnInfo().getSvrId()) ||
        (SVR_TYPE_MASTER == conn.getConnInfo().getSvrId()))
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
    if (SVR_TYPE_MONITOR == conn.getConnInfo().getSvrId())
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
    if (m_pBinLogMgr)
    {
        m_pBinLogMgr->commit(true);
        delete m_pBinLogMgr;
        m_pBinLogMgr = NULL;
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
		if (ullBinLogSize > CwxBinLogMgr::MAX_BINLOG_FILE_SIZE) ullBinLogSize = CwxBinLogMgr::MAX_BINLOG_FILE_SIZE;
        m_pBinLogMgr = new CwxBinLogMgr(m_config.getBinLog().m_strBinlogPath.c_str(),
            m_config.getBinLog().m_strBinlogPrex.c_str(),
            ullBinLogSize,
            m_config.getBinLog().m_bDelOutdayLogFile);
        if (0 != m_pBinLogMgr->init(m_config.getBinLog().m_uiMgrFileNum,
			m_config.getBinLog().m_bCache,
            CWX_TSS_2K_BUF))
        {///<���ʧ�ܣ��򷵻�-1
            CWX_ERROR(("Failure to init binlog manager, error:%s", CWX_TSS_2K_BUF));
            return -1;
        }
        m_bFirstBinLog = true;
        m_ttLastCommitTime = time(NULL);
        m_uiUnCommitLogNum = 0;
        ///��ȡsid
		m_uiCurSid = m_pBinLogMgr->getMaxSid() + CWX_MQ_MAX_BINLOG_FLUSH_COUNT + 1;
    }
    //��ʼ��MQ
    if (m_config.getMq().m_mq.getHostName().length() ||
        m_config.getMq().m_mq.getUnixDomain().length())
    {
        m_ttMqLastCommitTime = time(NULL);
        //��ʼ�����й�����
        if (m_queueMgr) delete m_queueMgr;
        m_queueMgr = new CwxMqQueueMgr(m_config.getMq().m_strLogFilePath,
            m_config.getMq().m_uiFlushNum);
        if (0 != m_queueMgr->init(m_pBinLogMgr))
        {
            CWX_ERROR(("Failure to init mq queue manager, err=%s", m_queueMgr->getErrMsg().c_str()));
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
        if (m_config.getMaster().m_recv.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_RECV, 
                m_config.getMaster().m_recv.getHostName().c_str(),
                m_config.getMaster().m_recv.getPort(),
                false,
                CWX_APP_MSG_MODE,
                CwxMqApp::setMasterRecvSockAttr,
                this))
            {
                CWX_ERROR(("Can't register the recv tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_recv.getHostName().c_str(),
                    m_config.getMaster().m_recv.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_recv.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_RECV,
                m_config.getMaster().m_recv.getUnixDomain().c_str(),
                false))
            {
                CWX_ERROR(("Can't register the recv unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_recv.getUnixDomain().c_str()));
                return -1;
            }
        }
        ///��binЭ���첽�ַ�
        if (m_config.getMaster().m_async.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC, 
                m_config.getMaster().m_async.getHostName().c_str(),
                m_config.getMaster().m_async.getPort(),
                false,
                CWX_APP_EVENT_MODE,
                CwxMqApp::setMasterDispatchSockAttr,
                this))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getMaster().m_async.getHostName().c_str(),
                    m_config.getMaster().m_async.getPort()));
                return -1;
            }
        }
        if (m_config.getMaster().m_async.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC,
                m_config.getMaster().m_async.getUnixDomain().c_str(),
                false,
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getMaster().m_async.getUnixDomain().c_str()));
                return -1;
            }
        }

    }
    else
    {
        ///����binЭ��master
        if (m_config.getSlave().m_master.getUnixDomain().length())
        {
            if (0 > this->noticeLsockConnect(SVR_TYPE_MASTER, 0, 
                m_config.getSlave().m_master.getUnixDomain().c_str(),
                false,
                1,
                2))
            {
                CWX_ERROR(("Can't register the master unix-domain connect: path-file=%s",
                    m_config.getSlave().m_master.getUnixDomain().c_str()));
                return -1;
            }
        }
        else if (m_config.getSlave().m_master.getHostName().length())
        {
            if (0 > this->noticeTcpConnect(SVR_TYPE_MASTER, 0, 
                m_config.getSlave().m_master.getHostName().c_str(),
                m_config.getSlave().m_master.getPort(),
                false,
                1,
                2,
                CwxMqApp::setSlaveReportSockAttr,
                this))
            {
                CWX_ERROR(("Can't register the master tcp connect: addr=%s, port=%d",
                    m_config.getSlave().m_master.getHostName().c_str(),
                    m_config.getSlave().m_master.getPort()));
                return -1;
            }
        }
        else
        {
            CWX_ERROR(("Not config master dispatch"));
            return -1;
        }
        ///binЭ���첽�ַ�
        if (m_config.getSlave().m_async.getHostName().length())
        {
            if (0 > this->noticeTcpListen(SVR_TYPE_ASYNC, 
                m_config.getSlave().m_async.getHostName().c_str(),
                m_config.getSlave().m_async.getPort(),
                false,
                CWX_APP_EVENT_MODE,
                CwxMqApp::setSlaveDispatchSockAttr,
                this))
            {
                CWX_ERROR(("Can't register the async-dispatch tcp accept listen: addr=%s, port=%d",
                    m_config.getSlave().m_async.getHostName().c_str(),
                    m_config.getSlave().m_async.getPort()));
                return -1;
            }
        }
        if (m_config.getSlave().m_async.getUnixDomain().length())
        {
            if (0 > this->noticeLsockListen(SVR_TYPE_ASYNC,
                m_config.getSlave().m_async.getUnixDomain().c_str(),
                false,
                CWX_APP_EVENT_MODE))
            {
                CWX_ERROR(("Can't register the async-dispatch unix-domain accept listen: path-file=%s",
                    m_config.getSlave().m_async.getUnixDomain().c_str()));
                return -1;
            }
        }
    }
    //��bin mq��ȡ�ļ����˿�
    if (m_config.getMq().m_mq.getHostName().length())
    {
        if (0 > this->noticeTcpListen(SVR_TYPE_FETCH, 
            m_config.getMq().m_mq.getHostName().c_str(),
            m_config.getMq().m_mq.getPort(),
            false,
            CWX_APP_EVENT_MODE,
            CwxMqApp::setMqSockAttr,
            this))
        {
            CWX_ERROR(("Can't register the mq-fetch tcp accept listen: addr=%s, port=%d",
                m_config.getMq().m_mq.getHostName().c_str(),
                m_config.getMq().m_mq.getPort()));
            return -1;
        }
    }
    if (m_config.getMq().m_mq.getUnixDomain().length())
    {
        if (0 > this->noticeLsockListen(SVR_TYPE_FETCH,
            m_config.getMq().m_mq.getUnixDomain().c_str(),
            false,
            CWX_APP_EVENT_MODE))
        {
            CWX_ERROR(("Can't register the mq-fetch unix-domain accept listen: path-file=%s",
                m_config.getMq().m_mq.getUnixDomain().c_str()));
            return -1;
        }
    }
    return 0;
}

int CwxMqApp::commit_mq()
{
    if (getMqLastCommitTime() + m_config.getMq().m_uiFlushSecond < (CWX_UINT32)time(NULL))
    {
        CWX_INFO(("Begin flush mq queue log file......."));
        getQueueMgr()->commit();
        setMqLastCommitTime(time(NULL));
        CWX_INFO(("End flush mq queue log file......."));
    }
    return 0;
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
    //���·���״̬
    updateAppRunState();
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
        char szSid1[64];
        char szSid2[64];
        list<CwxMqQueueInfo> queues;
        m_queueMgr->getQueuesInfo(queues);
        list<CwxMqQueueInfo>::iterator iter = queues.begin();
        char const* state="";
        while(iter != queues.end())
        {
            if (iter->m_ucQueueState==CwxBinLogMgr::CURSOR_STATE_UNSEEK)
                state = "unseek";
            else if (iter->m_ucQueueState==CwxBinLogMgr::CURSOR_STATE_ERROR)
                state = "error";
            else if (iter->m_ucQueueState==CwxBinLogMgr::CURSOR_STATE_READY)
                state = "ready";
            else
                state = "unknown";

            CwxCommon::toString(iter->m_ullCursorSid, szSid1, 10);
            CwxCommon::toString(iter->m_ullLeftNum, szSid2, 10);
            CwxCommon::snprintf(szLine, 4096, "STAT name(%s)|commit(%s)|def_timeout(%u)|max_timeout(%u)|cursor_state(%s)|log_state(%s)|sid(%s)|msg(%s)|subscribe(%s)|cursor_err(%s)|log_err(%s)\r\n",
                iter->m_strName.c_str(),
                iter->m_bCommit?"yes":"no",
				iter->m_uiDefTimeout,
				iter->m_uiMaxTimeout,
                state,
				iter->m_bQueueLogFileValid?"yes":"no",
                szSid1,
                szSid2,
                iter->m_strSubScribe.c_str(),
				iter->m_ucQueueState==CwxBinLogMgr::CURSOR_STATE_ERROR?iter->m_strQueueErrMsg.c_str():"",
				iter->m_bQueueLogFileValid?"":iter->m_strQueueLogFileErrMsg.c_str());
            MQ_MONITOR_APPEND();
            iter++;
        }
    }
    while(0);
    strcpy(m_szBuf + uiPos, "END\r\n");
    return strlen(m_szBuf);

}

///�ַ�channel���̺߳�����argΪapp����
void* CwxMqApp::DispatchThreadMain(CwxTss* , CwxMsgQueue* queue, void* arg)
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
        if (-1 == app->getAsyncDispChannel()->dispatch(1))
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
            CWX_ASSERT(block->event().getSvrId() == SVR_TYPE_ASYNC);

            if (channel->isRegIoHandle(block->event().getIoHandle()))
            {
                CWX_ERROR(("Handler[%] is register", block->event().getIoHandle()));
                break;
            }
            if (block->event().getSvrId() == SVR_TYPE_ASYNC)
            {
                handler = new CwxMqBinAsyncHandler(app, channel);
            }
            else
            {
                CWX_ERROR(("Invalid svr_type[%d], close handle[%d]", block->event().getSvrId(), block->event().getIoHandle()));
                ::close(block->event().getIoHandle());
            }
            handler->setHandle(block->event().getIoHandle());
            if (0 != handler->open())
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
void* CwxMqApp::MqThreadMain(CwxTss* , CwxMsgQueue* queue, void* arg)
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
        if (-1 == app->getMqChannel()->dispatch(1))
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
            if ((block->event().getEvent() == CwxEventInfo::CONN_CREATED) &&
                (block->event().getSvrId() == SVR_TYPE_FETCH))
            {
                if (channel->isRegIoHandle(block->event().getIoHandle()))
                {
                    CWX_ERROR(("Handler[%] is register", block->event().getIoHandle()));
                    break;
                }
                if (block->event().getSvrId() == SVR_TYPE_FETCH)
                {
                    handler = new CwxMqBinFetchHandler(app, channel);
                }
                else
                {
                    CWX_ERROR(("Invalid svr_type[%d], close handle[%d]", block->event().getSvrId(), block->event().getIoHandle()));
                    ::close(block->event().getIoHandle());
                }
                handler->setHandle(block->event().getIoHandle());
                if (0 != handler->open())
                {
                    CWX_ERROR(("Failure to register handler[%d]", handler->getHandle()));
                    delete handler;
                    break;
                }
            }
            else
            {
                CWX_ASSERT(block->event().getEvent() == CwxEventInfo::TIMEOUT_CHECK);
                CWX_ASSERT(block->event().getSvrId() == SVR_TYPE_FETCH);
                app->getQueueMgr()->checkTimeout(time(NULL));
                app->commit_mq();
            }
        } while(0);
        CwxMsgBlockAlloc::free(block);
        block = NULL;
    }
    if (queue->isDeactived()) return -1;
    return 0;
}

///����master recv���ӵ�����
int CwxMqApp::setMasterRecvSockAttr(CWX_HANDLE handle, void* arg)
{
    CwxMqApp* app = (CwxMqApp*)arg;
    if (app->getConfig().getMaster().m_recv.isKeepAlive())
    {
        if (0 != CwxSocket::setKeepalive(handle,
            true,
            CWX_APP_DEF_KEEPALIVE_IDLE,
            CWX_APP_DEF_KEEPALIVE_INTERNAL,
            CWX_APP_DEF_KEEPALIVE_COUNT))
        {
            CWX_ERROR(("Failure to set listen addr:%s, port:%u to keep-alive, errno=%d",
                app->getConfig().getMaster().m_recv.getHostName().c_str(),
                app->getConfig().getMaster().m_recv.getPort(),
                errno));
            return -1;
        }
    }

    int flags= 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u NODELAY, errno=%d",
            app->getConfig().getMaster().m_recv.getHostName().c_str(),
            app->getConfig().getMaster().m_recv.getPort(),
            errno));
        return -1;
    }
    struct linger ling= {0, 0};
    if (setsockopt(handle, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u LINGER, errno=%d",
            app->getConfig().getMaster().m_recv.getHostName().c_str(),
            app->getConfig().getMaster().m_recv.getPort(),
            errno));
        return -1;
    }
    return 0;

}
///����master dispatch���ӵ�����
int CwxMqApp::setMasterDispatchSockAttr(CWX_HANDLE handle, void* arg)
{
    CwxMqApp* app = (CwxMqApp*)arg;
    if (app->getConfig().getCommon().m_uiSockBufSize)
    {
        int iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while (setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (void*)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
        iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while(setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (void *)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
    }

    if (app->getConfig().getMaster().m_async.isKeepAlive())
    {
        if (0 != CwxSocket::setKeepalive(handle,
            true,
            CWX_APP_DEF_KEEPALIVE_IDLE,
            CWX_APP_DEF_KEEPALIVE_INTERNAL,
            CWX_APP_DEF_KEEPALIVE_COUNT))
        {
            CWX_ERROR(("Failure to set listen addr:%s, port:%u to keep-alive, errno=%d",
                app->getConfig().getMaster().m_async.getHostName().c_str(),
                app->getConfig().getMaster().m_async.getPort(),
                errno));
            return -1;
        }
    }

    int flags= 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u NODELAY, errno=%d",
            app->getConfig().getMaster().m_async.getHostName().c_str(),
            app->getConfig().getMaster().m_async.getPort(),
            errno));
        return -1;
    }
    struct linger ling= {0, 0};
    if (setsockopt(handle, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u LINGER, errno=%d",
            app->getConfig().getMaster().m_async.getHostName().c_str(),
            app->getConfig().getMaster().m_async.getPort(),
            errno));
        return -1;
    }
    return 0;
}
///����slave dispatch���ӵ�����
int CwxMqApp::setSlaveDispatchSockAttr(CWX_HANDLE handle, void* arg)
{
    CwxMqApp* app = (CwxMqApp*)arg;
    if (app->getConfig().getCommon().m_uiSockBufSize)
    {
        int iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while (setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (void*)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
        iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while(setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (void *)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
    }

    if (app->getConfig().getSlave().m_async.isKeepAlive())
    {
        if (0 != CwxSocket::setKeepalive(handle,
            true,
            CWX_APP_DEF_KEEPALIVE_IDLE,
            CWX_APP_DEF_KEEPALIVE_INTERNAL,
            CWX_APP_DEF_KEEPALIVE_COUNT))
        {
            CWX_ERROR(("Failure to set listen addr:%s, port:%u to keep-alive, errno=%d",
                app->getConfig().getSlave().m_async.getHostName().c_str(),
                app->getConfig().getSlave().m_async.getPort(),
                errno));
            return -1;
        }
    }

    int flags= 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u NODELAY, errno=%d",
            app->getConfig().getSlave().m_async.getHostName().c_str(),
            app->getConfig().getSlave().m_async.getPort(),
            errno));
        return -1;
    }
    struct linger ling= {0, 0};
    if (setsockopt(handle, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u LINGER, errno=%d",
            app->getConfig().getSlave().m_async.getHostName().c_str(),
            app->getConfig().getSlave().m_async.getPort(),
            errno));
        return -1;
    }
    return 0;
}

///����slave report���ӵ���Ϥ
int CwxMqApp::setSlaveReportSockAttr(CWX_HANDLE handle, void* arg)
{
    CwxMqApp* app = (CwxMqApp*)arg;
    if (app->getConfig().getCommon().m_uiSockBufSize)
    {
        int iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while (setsockopt(handle, SOL_SOCKET, SO_SNDBUF, (void*)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
        iSockBuf = (app->getConfig().getCommon().m_uiSockBufSize + 1023)/1024;
        iSockBuf *= 1024;
        while(setsockopt(handle, SOL_SOCKET, SO_RCVBUF, (void *)&iSockBuf, sizeof(iSockBuf)) < 0)
        {
            iSockBuf -= 1024;
            if (iSockBuf <= 1024) break;
        }
    }

    if (app->getConfig().getSlave().m_master.isKeepAlive())
    {
        if (0 != CwxSocket::setKeepalive(handle,
            true,
            CWX_APP_DEF_KEEPALIVE_IDLE,
            CWX_APP_DEF_KEEPALIVE_INTERNAL,
            CWX_APP_DEF_KEEPALIVE_COUNT))
        {
            CWX_ERROR(("Failure to set listen addr:%s, port:%u to keep-alive, errno=%d",
                app->getConfig().getSlave().m_master.getHostName().c_str(),
                app->getConfig().getSlave().m_master.getPort(),
                errno));
            return -1;
        }
    }

    int flags= 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u NODELAY, errno=%d",
            app->getConfig().getSlave().m_master.getHostName().c_str(),
            app->getConfig().getSlave().m_master.getPort(),
            errno));
        return -1;
    }
    struct linger ling= {0, 0};
    if (setsockopt(handle, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u LINGER, errno=%d",
            app->getConfig().getSlave().m_master.getHostName().c_str(),
            app->getConfig().getSlave().m_master.getPort(),
            errno));
        return -1;
    }
    return 0;

}
///����mq���ӵ���Ϥ
int CwxMqApp::setMqSockAttr(CWX_HANDLE handle, void* arg)
{
    CwxMqApp* app = (CwxMqApp*)arg;

    if (app->getConfig().getMq().m_mq.isKeepAlive())
    {
        if (0 != CwxSocket::setKeepalive(handle,
            true,
            CWX_APP_DEF_KEEPALIVE_IDLE,
            CWX_APP_DEF_KEEPALIVE_INTERNAL,
            CWX_APP_DEF_KEEPALIVE_COUNT))
        {
            CWX_ERROR(("Failure to set listen addr:%s, port:%u to keep-alive, errno=%d",
                app->getConfig().getMq().m_mq.getHostName().c_str(),
                app->getConfig().getMq().m_mq.getPort(),
                errno));
            return -1;
        }
    }

    int flags= 1;
    if (setsockopt(handle, IPPROTO_TCP, TCP_NODELAY, (void *)&flags, sizeof(flags)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u NODELAY, errno=%d",
            app->getConfig().getMq().m_mq.getHostName().c_str(),
            app->getConfig().getMq().m_mq.getPort(),
            errno));
        return -1;
    }
    struct linger ling= {0, 0};
    if (setsockopt(handle, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling)) != 0)
    {
        CWX_ERROR(("Failure to set listen addr:%s, port:%u LINGER, errno=%d",
            app->getConfig().getMq().m_mq.getHostName().c_str(),
            app->getConfig().getMq().m_mq.getPort(),
            errno));
        return -1;
    }
    return 0;
}
