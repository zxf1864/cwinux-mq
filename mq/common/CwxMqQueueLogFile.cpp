#include "CwxMqQueueLogFile.h"
#include "CwxDate.h"

CwxMqQueueLogFile::CwxMqQueueLogFile(CWX_UINT32 uiFsyncInternal,
                                     string const& strFileName)
{
    m_strFileName = strFileName; ///<系统文件名字
    m_strOldFileName = strFileName + ".old";///<旧系统文件名字
    m_strNewFileName = strFileName + ".new"; ///<新系统文件的名字
    m_fd = NULL;
    m_bLock = false;
    m_uiFsyncInternal = uiFsyncInternal; ///<flush硬盘的间隔
    m_uiCurLogCount = 0; ///<自上次fsync来，log记录的次数
    m_uiTotalLogCount = 0; ///<当前文件log的数量
	m_uiLastSaveTime = 0;
    strcpy(m_szErr2K, "No init");
}

CwxMqQueueLogFile::~CwxMqQueueLogFile()
{
    closeFile(true);
}

///初始化系统文件；0：成功；-1：失败
int CwxMqQueueLogFile::init(CwxMqQueueInfo& queue,
         set<CWX_UINT64>& uncommitSets,
         set<CWX_UINT64>& commitSets)
{
    if (0 != prepare())
    {
        closeFile(false);
        return -1;
    }
    //清空数据
	queue.m_strName.erase();
	uncommitSets.clear();
    commitSets.clear();
    //加载数据
    if (0 != load(queue, uncommitSets, commitSets))
    {
        //若失败，清空数据
		uncommitSets.clear();
		commitSets.clear();
        closeFile(false);
        return -1;
    }
	m_uiLastSaveTime = time(NULL);
    return 0;
}

///保存队列信息；0：成功；-1：失败
int CwxMqQueueLogFile::save(CwxMqQueueInfo const& queue,
                            set<CWX_UINT64>const& uncommitSets,
                            set<CWX_UINT64>const& commitSets)
{
    if (!m_fd) return -1;
    //写新文件
    int fd = ::open(m_strNewFileName.c_str(),  O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (-1 == fd)
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open new sys file:%s, errno=%d",
            m_strNewFileName.c_str(),
            errno);
        closeFile(true);
        return -1;
    }
    //写入队列信息
    char line[1024];
    char szSid[64];
    ssize_t len = 0;
    //queue:name=q1|sid=12345|commit=true|def_timeout=5|max_timeout=300|user=u_q1|passwd=p_q1|subcribe=*
	len = CwxCommon::snprintf(line, 
            1023,
            "%s:name=%s|sid=%s|commit=%s|def_timeout=%u|max_timeout=%u|user=%s|passwd=%s|subscribe=%s\n",
            CWX_MQ_QUEUE,
            queue.m_strName.c_str(),
            CwxCommon::toString(queue.m_ullCursorSid, szSid, 10),
            queue.m_bCommit?"true":"false",
            queue.m_uiDefTimeout,
            queue.m_uiMaxTimeout,
            queue.m_strUser.c_str(),
            queue.m_strPasswd.c_str(),
            queue.m_strSubScribe.c_str());
	if (len != write(fd, line, len))
	{
		CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write new sys file:%s, errno=%d",
			m_strNewFileName.c_str(),
			errno);
		closeFile(true);
		return -1;
	}
    //写未提交的sid
    set<CWX_UINT64>::const_iterator iter_sid = uncommitSets.begin();
    while(iter_sid != uncommitSets.end())
    {
		//uncommit:sid=1
		len = CwxCommon::snprintf(line, 1023, "%s:sid=%s\n",
			CWX_MQ_UNCOMMIT,
			CwxCommon::toString(*iter_sid, szSid, 10));
		if (len != write(fd, line, len))
		{
			CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write new sys file:%s, errno=%d",
				m_strNewFileName.c_str(),
				errno);
			closeFile(true);
			return -1;
		}
		iter_sid++;
    }
    //写提交的sid
    iter_sid = commitSets.begin();
    while(iter_sid != commitSets.end())
    {
		//commit:sid=1
		len = CwxCommon::snprintf(line, 1023, "%s:sid=%s\n",
			CWX_MQ_COMMIT,
			CwxCommon::toString(*iter_sid, szSid, 10));
		if (len != write(fd, line, len))
		{
			CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write new sys file:%s, errno=%d",
				m_strNewFileName.c_str(),
				errno);
			closeFile(true);
			return -1;
		}
		iter_sid++;
    }
    ::fsync(fd);
    ::close(fd);
    //确保旧文件删除
    if (CwxFile::isFile(m_strOldFileName.c_str()) &&
        !CwxFile::rmFile(m_strOldFileName.c_str()))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to rm old sys file:%s, errno=%d",
            m_strOldFileName.c_str(),
            errno);
        closeFile(true);
        return -1;
    }
    //关闭当前文件
    closeFile(true);
    //将当前文件move为old文件
    if (!CwxFile::moveFile(m_strFileName.c_str(), m_strOldFileName.c_str()))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move current sys file:%s to old sys file:%s, errno=%d",
            m_strFileName.c_str(),
            m_strOldFileName.c_str(),
            errno);
        return -1;
    }
    //将新文件移为当前文件
    if (!CwxFile::moveFile(m_strNewFileName.c_str(), m_strFileName.c_str()))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move new sys file:%s to current sys file:%s, errno=%d",
            m_strNewFileName.c_str(),
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    //打开当前文件，接受写
    //open file
    m_fd = ::fopen(m_strFileName.c_str(), "a+");
    if (!m_fd)
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    if (!CwxFile::lock(fileno(m_fd)))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to lock sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    m_bLock = true;
    m_uiTotalLogCount = 0;
    m_uiCurLogCount = 0;
	m_uiLastSaveTime = time(NULL);
    return 0;
}

///写commit记录；-1：失败；否则返回已经写入的log数量
int CwxMqQueueLogFile::log(CWX_UINT64 sid)
{
    if (m_fd)
    {
        char szBuf[1024];
        char szSid[64];
		size_t len = CwxCommon::snprintf(szBuf, 1023, "%s:sid=%s\n", CWX_MQ_COMMIT, CwxCommon::toString(sid, szSid, 10));
        if (len != fwrite(szBuf, 1, len, m_fd))
        {
            closeFile(false);
            CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write log to file[%s], errno=%d",
                m_strFileName.c_str(),
                errno);
            return -1;
        }
        m_uiCurLogCount++;
        m_uiTotalLogCount++;
        if (m_uiCurLogCount >= m_uiFsyncInternal)
        {
            if (0 != fsync()) return -1;
        }
        return m_uiTotalLogCount;
    }
    return -1;
}
///强行fsync日志文件；0：成功；-1：失败
int CwxMqQueueLogFile::fsync()
{
    if (m_uiCurLogCount && m_fd)
    {
        fflush(m_fd);
        if (0 != ::fsync(fileno(m_fd)))
        {
            CwxCommon::snprintf(m_szErr2K, 2047, "Failure to fsync file[%s], errno=%d",
                m_strFileName.c_str(),
                errno);
            closeFile(false);
            return -1;
        }
        m_uiCurLogCount = 0;
    }
    return 0;
}


int CwxMqQueueLogFile::load(CwxMqQueueInfo& queue,
                            set<CWX_UINT64>& uncommitSets,
                            set<CWX_UINT64>& commitSets)
{

    bool bRet = true;
    string line;
    string strQueuePrex=CWX_MQ_QUEUE;
    string strCommitPrex = CWX_MQ_COMMIT;
    string strUncommitPrex = CWX_MQ_UNCOMMIT;
    strQueuePrex +=":";
    strCommitPrex += ":";
    strUncommitPrex +=":";
    //seek到文件头部
    fseek(m_fd, 0, SEEK_SET);
    //step
    int step = 0; //0:load queue, 1:load uncommit; 2:load commit
    m_uiLine = 0;
    string strQueue;
    CWX_UINT64 ullSid;
    while((bRet = CwxFile::readTxtLine(m_fd, line)))
    {
        if (line.empty()) break;
        m_uiLine++;
        if (0 == step)
        {//queue:name=q1|sid=12345|commit=true|def_timeout=5|max_timeout=300|user=u_q1|passwd=p_q1|subcribe=*
			line = line.substr(strQueuePrex.length());
			if (0 != parseQueue(line, queue))
			{
				return -1;
			}
            step = 1;
			continue; ///读取下一行
        }
        if (1 == step)
        {//uncommit:sid=1
            if (strUncommitPrex == line.substr(0, strUncommitPrex.length()))
            {
                line = line.substr(strUncommitPrex.length());
                if (0 != parseSid(line, ullSid))
                {
                    return -1;
                }
				uncommitSets.insert(ullSid);
                continue;
            }
            step = 2;
        }
        if (2 == step)
        {//commit:sid=1
            if (strCommitPrex == line.substr(0, strCommitPrex.length()))
            {
                line = line.substr(strCommitPrex.length());
                if (0 != parseSid(line, ullSid))
                {
                    return -1;
                }
                commitSets.insert(ullSid);
                //如果sid在uncommit set中存在，则需要删除
				if (uncommitSets.find(ullSid) != uncommitSets.end())
				{
					uncommitSets.erase(ullSid);
                }
                m_uiTotalLogCount++;
                continue;
            }
            ///未知的log日志
            CwxCommon::snprintf(m_szErr2K, 2047, "Unknown log:%s, line:%d",
                line.c_str(),
                m_uiLine);
            return -1;
        }
    }
    if (!bRet)
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to read sys file[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    return 0;

}


int CwxMqQueueLogFile::parseQueue(string const& line, CwxMqQueueInfo& queue)
{
    list<pair<string, string> > items;
    pair<string, string> item;
    CwxCommon::split(line, items, '|');
    //get name
    if (!CwxCommon::findKey(items, CWX_MQ_NAME, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue has no [%s] key, line:%u", 
            CWX_MQ_NAME,
            m_uiLine);
        return -1;
    }
    queue.m_strName = item.second;
    //get sid
    if (!CwxCommon::findKey(items, CWX_MQ_SID, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_SID,
            m_uiLine);
        return -1;
    }
    queue.m_ullCursorSid = strtoull(item.second.c_str(), NULL, 0);
    //get commit
    if (!CwxCommon::findKey(items, CWX_MQ_COMMIT, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_COMMIT,
            m_uiLine);
        return -1;
    }
    queue.m_bCommit = item.second=="true"?true:false;
    //get def_timeout
    if (!CwxCommon::findKey(items, CWX_MQ_DEF_TIMEOUT, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_DEF_TIMEOUT,
            m_uiLine);
        return -1;
    }
    queue.m_uiDefTimeout = strtoul(item.second.c_str(), NULL, 0);
    if (queue.m_uiDefTimeout < CWX_MQ_MIN_TIMEOUT_SECOND) queue.m_uiDefTimeout = CWX_MQ_MIN_TIMEOUT_SECOND;
    if (queue.m_uiDefTimeout > CWX_MQ_MAX_TIMEOUT_SECOND) queue.m_uiDefTimeout = CWX_MQ_MAX_TIMEOUT_SECOND;
    //get max_timeout
    if (!CwxCommon::findKey(items, CWX_MQ_MAX_TIMEOUT, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_MAX_TIMEOUT,
            m_uiLine);
        return -1;
    }
    queue.m_uiDefTimeout = strtoul(item.second.c_str(), NULL, 0);
    if (queue.m_uiMaxTimeout < CWX_MQ_MIN_TIMEOUT_SECOND) queue.m_uiMaxTimeout = CWX_MQ_MIN_TIMEOUT_SECOND;
    if (queue.m_uiMaxTimeout > CWX_MQ_MAX_TIMEOUT_SECOND) queue.m_uiMaxTimeout = CWX_MQ_MAX_TIMEOUT_SECOND;
    //get user
    if (!CwxCommon::findKey(items, CWX_MQ_USER, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_USER,
            m_uiLine);
        return -1;
    }
    queue.m_strUser = item.second;
    //get passwd
    if (!CwxCommon::findKey(items, CWX_MQ_PASSWD, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_PASSWD,
            m_uiLine);
        return -1;
    }
    queue.m_strPasswd = item.second;
    //get scribe
    if (!CwxCommon::findKey(items, CWX_MQ_SUBSCRIBE, item))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_SUBSCRIBE,
            m_uiLine);
        return -1;
    }
    queue.m_strSubScribe = item.second;
    string errMsg;
    if (!CwxMqPoco::isValidSubscribe(queue.m_strSubScribe, errMsg))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s]'s subscribe[%s] is invalid, err=%s line:%u",
            queue.m_strName.c_str(),
            queue.m_strSubScribe.c_str(),
            errMsg.c_str(),
            m_uiLine);
        return -1;

    }
    return 0;
}

int CwxMqQueueLogFile::parseSid(string const& line, CWX_UINT64& ullSid)
{
    pair<string, string> item;
    if (!CwxCommon::keyValue(line, item))
	{
		CwxCommon::snprintf(m_szErr2K, 2047, "Not find [%s] key, line:%u", 
			CWX_MQ_SID,
			m_uiLine);
		return -1;
	}
	if (item.first != CWX_MQ_SID)
	{
		CwxCommon::snprintf(m_szErr2K, 2047, "Not find [%s] key, line:%u", 
			CWX_MQ_SID,
			m_uiLine);
		return -1;
	}
    //get sid
    ullSid = strtoull(item.second.c_str(), NULL, 0);
    return 0;
}


int CwxMqQueueLogFile::prepare()
{
    bool bExistOld = CwxFile::isFile(m_strOldFileName.c_str());
    bool bExistCur = CwxFile::isFile(m_strFileName.c_str());
    bool bExistNew = CwxFile::isFile(m_strNewFileName.c_str());

    if (m_fd)
    {
        closeFile(true);
    }
    m_fd = NULL;
    m_uiCurLogCount = 0; ///<自上次fsync来，log记录的次数
    m_uiTotalLogCount = 0; ///<当前文件log的数量
    strcpy(m_szErr2K, "No init");

    if (!bExistCur)
    {
        if (bExistOld)
        {//采用旧文件
            if (!CwxFile::moveFile(m_strOldFileName.c_str(), m_strFileName.c_str()))
            {
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move old sys file[%s] to cur sys file:[%s], errno=%d",
                    m_strOldFileName.c_str(),
                    m_strFileName.c_str(),
                    errno);
                return -1;
            }
        }
        else if(bExistNew)
        {//采用新文件
            if (!CwxFile::moveFile(m_strNewFileName.c_str(), m_strFileName.c_str()))
            {
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move new sys file[%s] to cur sys file:[%s], errno=%d",
                    2047,
                    m_strNewFileName.c_str(),
                    m_strFileName.c_str(),
                    errno);
                return -1;
            }
        }
        else
        {//创建空的当前文件
            int fd = ::open(m_strFileName.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (-1 == fd)
            {
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to create cur sys file:[%s], errno=%d",
                    m_strFileName.c_str(),
                    errno);
                return -1;
            }
            ::close(fd);
        }
    }
    //open file
    m_fd = ::fopen(m_strFileName.c_str(), "a+");
    if (!m_fd)
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    if (!CwxFile::lock(fileno(m_fd)))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to lock sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    m_bLock = true;
    return 0;
}
