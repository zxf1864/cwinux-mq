#include "CwxMqQueueLogFile.h"
#include "CwxDate.h"

CwxMqQueueLogFile::CwxMqQueueLogFile(CWX_UINT32 uiFsyncInternal,
                                     string const& strFileName)
{
    m_strFileName = strFileName; ///<ϵͳ�ļ�����
    m_strOldFileName = strFileName + ".old";///<��ϵͳ�ļ�����
    m_strNewFileName = strFileName + ".new"; ///<��ϵͳ�ļ�������
    m_fd = NULL;
    m_bLock = false;
    m_uiFsyncInternal = uiFsyncInternal; ///<flushӲ�̵ļ��
    m_uiCurLogCount = 0; ///<���ϴ�fsync����log��¼�Ĵ���
    m_uiTotalLogCount = 0; ///<��ǰ�ļ�log������
	m_uiLastSaveTime = 0;
    strcpy(m_szErr2K, "No init");
}

CwxMqQueueLogFile::~CwxMqQueueLogFile(){
    closeFile(true);
}

///��ʼ��ϵͳ�ļ���0���ɹ���-1��ʧ��
int CwxMqQueueLogFile::init(CwxMqQueueInfo& queue,
         set<CWX_UINT64>& uncommitSets,
         set<CWX_UINT64>& commitSets)
{
    if (0 != prepare()){
        closeFile(false);
        return -1;
    }
    //�������
	queue.m_strName.erase();
	uncommitSets.clear();
    commitSets.clear();
    //��������
    if (0 != load(queue, uncommitSets, commitSets)){
        //��ʧ�ܣ��������
		uncommitSets.clear();
		commitSets.clear();
        closeFile(false);
        return -1;
    }
	m_uiLastSaveTime = time(NULL);
    return 0;
}

///���������Ϣ��0���ɹ���-1��ʧ��
int CwxMqQueueLogFile::save(CwxMqQueueInfo const& queue,
                            set<CWX_UINT64>const& uncommitSets,
                            set<CWX_UINT64>const& commitSets)
{
    if (!m_fd) return -1;
    //д���ļ�
    int fd = ::open(m_strNewFileName.c_str(),  O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (-1 == fd){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open new sys file:%s, errno=%d",
            m_strNewFileName.c_str(),
            errno);
        closeFile(true);
        return -1;
    }
    //д�������Ϣ
    char line[1024];
    char szSid[64];
    ssize_t len = 0;
    //queue:name=q1|sid=12345|u=u_q1|p=p_q1|subscribe=*
	len = CwxCommon::snprintf(line, 
            1023,
            "%s:name=%s|sid=%s|u=%s|p=%s|subscribe=%s\n",
            CWX_MQ_Q,
            queue.m_strName.c_str(),
            CwxCommon::toString(queue.m_ullCursorSid, szSid, 10),
            queue.m_strUser.c_str(),
            queue.m_strPasswd.c_str(),
            queue.m_strSubScribe.c_str());
	if (len != write(fd, line, len)){
		CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write new sys file:%s, errno=%d",
			m_strNewFileName.c_str(),
			errno);
		closeFile(true);
		return -1;
	}
    //дδ�ύ��sid
    set<CWX_UINT64>::const_iterator iter_sid = uncommitSets.begin();
    while(iter_sid != uncommitSets.end()){
		//uncommit:sid=1
		len = CwxCommon::snprintf(line, 1023, "%s:sid=%s\n",
			CWX_MQ_UNCOMMIT,
			CwxCommon::toString(*iter_sid, szSid, 10));
		if (len != write(fd, line, len)){
			CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write new sys file:%s, errno=%d",
				m_strNewFileName.c_str(),
				errno);
			closeFile(true);
			return -1;
		}
		iter_sid++;
    }
    //д�ύ��sid
    iter_sid = commitSets.begin();
    while(iter_sid != commitSets.end()){
		//commit:sid=1
		len = CwxCommon::snprintf(line, 1023, "%s:sid=%s\n",
			CWX_MQ_COMMIT,
			CwxCommon::toString(*iter_sid, szSid, 10));
		if (len != write(fd, line, len)){
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
    //ȷ�����ļ�ɾ��
    if (CwxFile::isFile(m_strOldFileName.c_str()) &&
        !CwxFile::rmFile(m_strOldFileName.c_str()))
    {
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to rm old sys file:%s, errno=%d",
            m_strOldFileName.c_str(),
            errno);
        closeFile(true);
        return -1;
    }
    //�رյ�ǰ�ļ�
    closeFile(true);
    //����ǰ�ļ�moveΪold�ļ�
    if (!CwxFile::moveFile(m_strFileName.c_str(), m_strOldFileName.c_str())){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move current sys file:%s to old sys file:%s, errno=%d",
            m_strFileName.c_str(),
            m_strOldFileName.c_str(),
            errno);
        return -1;
    }
    //�����ļ���Ϊ��ǰ�ļ�
    if (!CwxFile::moveFile(m_strNewFileName.c_str(), m_strFileName.c_str())){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move new sys file:%s to current sys file:%s, errno=%d",
            m_strNewFileName.c_str(),
            m_strFileName.c_str(),
            errno);
        return -1;
    }
	//ɾ�����ļ�
	CwxFile::rmFile(m_strOldFileName.c_str());
    //�򿪵�ǰ�ļ�������д
    //open file
    m_fd = ::fopen(m_strFileName.c_str(), "a+");
    if (!m_fd){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    if (!CwxFile::lock(fileno(m_fd))){
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

///дcommit��¼��-1��ʧ�ܣ����򷵻��Ѿ�д���log����
int CwxMqQueueLogFile::log(CWX_UINT64 sid){
    if (m_fd){
        char szBuf[1024];
        char szSid[64];
		size_t len = CwxCommon::snprintf(szBuf, 1023, "%s:sid=%s\n", CWX_MQ_COMMIT, CwxCommon::toString(sid, szSid, 10));
        if (len != fwrite(szBuf, 1, len, m_fd)){
            closeFile(false);
            CwxCommon::snprintf(m_szErr2K, 2047, "Failure to write log to file[%s], errno=%d",
                m_strFileName.c_str(),
                errno);
            return -1;
        }
        m_uiCurLogCount++;
        m_uiTotalLogCount++;
        if (m_uiCurLogCount >= m_uiFsyncInternal){
            if (0 != fsync()) return -1;
        }
        return m_uiTotalLogCount;
    }
    return -1;
}
///ǿ��fsync��־�ļ���0���ɹ���-1��ʧ��
int CwxMqQueueLogFile::fsync(){
    if (m_uiCurLogCount && m_fd){
        fflush(m_fd);
        if (0 != ::fsync(fileno(m_fd))){
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
                            set<CWX_UINT64>& commitSets){

    bool bRet = true;
    string line;
    string strQueuePrex=CWX_MQ_Q;
    string strCommitPrex = CWX_MQ_COMMIT;
    string strUncommitPrex = CWX_MQ_UNCOMMIT;
    strQueuePrex +=":";
    strCommitPrex += ":";
    strUncommitPrex +=":";
    //seek���ļ�ͷ��
    fseek(m_fd, 0, SEEK_SET);
    //step
    int step = 0; //0:load queue, 1:load uncommit; 2:load commit
    m_uiLine = 0;
    string strQueue;
    CWX_UINT64 ullSid;
    while((bRet = CwxFile::readTxtLine(m_fd, line))){
        if (line.empty()) break;
        m_uiLine++;
        if (0 == step){//queue:name=q1|sid=12345|u=u_q1|p=p_q1|subscribe=*
			line = line.substr(strQueuePrex.length());
			if (0 != parseQueue(line, queue)){
				return -1;
			}
            step = 1;
			continue; ///��ȡ��һ��
        }
        if (1 == step){//uncommit:sid=1
            if (strUncommitPrex == line.substr(0, strUncommitPrex.length())){
                line = line.substr(strUncommitPrex.length());
                if (0 != parseSid(line, ullSid)){
                    return -1;
                }
				uncommitSets.insert(ullSid);
                continue;
            }
            step = 2;
        }
        if (2 == step){//commit:sid=1
            if (strCommitPrex == line.substr(0, strCommitPrex.length())){
                line = line.substr(strCommitPrex.length());
                if (0 != parseSid(line, ullSid)){
                    return -1;
                }
                commitSets.insert(ullSid);
                //���sid��uncommit set�д��ڣ�����Ҫɾ��
				if (uncommitSets.find(ullSid) != uncommitSets.end()){
					uncommitSets.erase(ullSid);
                }
                m_uiTotalLogCount++;
                continue;
            }
            ///δ֪��log��־
            CwxCommon::snprintf(m_szErr2K, 2047, "Unknown log:%s, line:%d",
                line.c_str(),
                m_uiLine);
            return -1;
        }
    }
    if (!bRet){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to read sys file[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    return 0;

}


int CwxMqQueueLogFile::parseQueue(string const& line, CwxMqQueueInfo& queue){
    list<pair<string, string> > items;
    pair<string, string> item;
    CwxCommon::split(line, items, '|');
    //get name
    if (!CwxCommon::findKey(items, CWX_MQ_NAME, item)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue has no [%s] key, line:%u", 
            CWX_MQ_NAME,
            m_uiLine);
        return -1;
    }
    queue.m_strName = item.second;
    //get sid
    if (!CwxCommon::findKey(items, CWX_MQ_SID, item)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_SID,
            m_uiLine);
        return -1;
    }
    queue.m_ullCursorSid = strtoull(item.second.c_str(), NULL, 10);
    //get user
    if (!CwxCommon::findKey(items, CWX_MQ_U, item)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_U,
            m_uiLine);
        return -1;
    }
    queue.m_strUser = item.second;
    //get passwd
    if (!CwxCommon::findKey(items, CWX_MQ_P, item)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_P,
            m_uiLine);
        return -1;
    }
    queue.m_strPasswd = item.second;
    //get scribe
    if (!CwxCommon::findKey(items, CWX_MQ_SUBSCRIBE, item)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s] has no [%s] key, line:%u",
            queue.m_strName.c_str(),
            CWX_MQ_SUBSCRIBE,
            m_uiLine);
        return -1;
    }
    queue.m_strSubScribe = item.second;
    string errMsg;
    if (!CwxMqPoco::isValidSubscribe(queue.m_strSubScribe, errMsg)){
        CwxCommon::snprintf(m_szErr2K, 2047, "queue[%s]'s subscribe[%s] is invalid, err=%s line:%u",
            queue.m_strName.c_str(),
            queue.m_strSubScribe.c_str(),
            errMsg.c_str(),
            m_uiLine);
        return -1;

    }
    return 0;
}

int CwxMqQueueLogFile::parseSid(string const& line, CWX_UINT64& ullSid){
    pair<string, string> item;
    if (!CwxCommon::keyValue(line, item)){
		CwxCommon::snprintf(m_szErr2K, 2047, "Not find [%s] key, line:%u", 
			CWX_MQ_SID,
			m_uiLine);
		return -1;
	}
	if (item.first != CWX_MQ_SID){
		CwxCommon::snprintf(m_szErr2K, 2047, "Not find [%s] key, line:%u", 
			CWX_MQ_SID,
			m_uiLine);
		return -1;
	}
    //get sid
    ullSid = strtoull(item.second.c_str(), NULL, 10);
    return 0;
}


int CwxMqQueueLogFile::prepare(){
    bool bExistOld = CwxFile::isFile(m_strOldFileName.c_str());
    bool bExistCur = CwxFile::isFile(m_strFileName.c_str());
    bool bExistNew = CwxFile::isFile(m_strNewFileName.c_str());

    if (m_fd){
        closeFile(true);
    }
    m_fd = NULL;
    m_uiCurLogCount = 0; ///<���ϴ�fsync����log��¼�Ĵ���
    m_uiTotalLogCount = 0; ///<��ǰ�ļ�log������
    strcpy(m_szErr2K, "No init");

    if (!bExistCur){
        if (bExistOld){//���þ��ļ�
            if (!CwxFile::moveFile(m_strOldFileName.c_str(), m_strFileName.c_str())){
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move old sys file[%s] to cur sys file:[%s], errno=%d",
                    m_strOldFileName.c_str(),
                    m_strFileName.c_str(),
                    errno);
                return -1;
            }
        }else if(bExistNew){//�������ļ�
            if (!CwxFile::moveFile(m_strNewFileName.c_str(), m_strFileName.c_str())){
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to move new sys file[%s] to cur sys file:[%s], errno=%d",
                    2047,
                    m_strNewFileName.c_str(),
                    m_strFileName.c_str(),
                    errno);
                return -1;
            }
        }else{//�����յĵ�ǰ�ļ�
            int fd = ::open(m_strFileName.c_str(), O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
            if (-1 == fd){
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
    if (!m_fd){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to open sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
    if (!CwxFile::lock(fileno(m_fd))){
        CwxCommon::snprintf(m_szErr2K, 2047, "Failure to lock sys file:[%s], errno=%d",
            m_strFileName.c_str(),
            errno);
        return -1;
    }
	//ɾ�����ļ�
	CwxFile::rmFile(m_strOldFileName.c_str());
	//ɾ�����ļ�
	CwxFile::rmFile(m_strNewFileName.c_str());
    m_bLock = true;
    return 0;
}
