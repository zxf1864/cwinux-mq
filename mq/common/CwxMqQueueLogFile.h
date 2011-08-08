#ifndef __CWX_MQ_QUEUE_LOG_FILE_H__
#define __CWX_MQ_QUEUE_LOG_FILE_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxMqMacro.h"
#include "CwxMutexLock.h"
#include "CwxStl.h"
#include "CwxStlFunc.h"
#include "CwxCommon.h"
#include "CwxFile.h"
#include "CwxDate.h"
#include "CwxMqDef.h"

/**
@file CwxMqQueueLogFile.h
@brief MQ Queue ���е�log�����塣
@author cwinux@gmail.com
@version 1.0
@date 2011-05-27
@warning
@bug
*/


class CwxMqQueueLogFile
{
public:
    enum
    {
        SWITCH_FILE_LOG_NUM = 100000, ///<д����ٸ�Log��¼����Ҫ�л���־�ļ�
    };
public:
    CwxMqQueueLogFile(CWX_UINT32 uiFsyncInternal, string const& strFileName);
    ~CwxMqQueueLogFile();
public:
    ///��ʼ��ϵͳ�ļ���0���ɹ���-1��ʧ��
    int init(CwxMqQueueInfo& queue,
        set<CWX_UINT64>& uncommitSets,
        set<CWX_UINT64>& commitSets);
    ///���������Ϣ��0���ɹ���-1��ʧ��
    int save(CwxMqQueueInfo const& queue,
        set<CWX_UINT64> const& uncommitSets,
        set<CWX_UINT64> const& commitSets);
    ///дcommit��¼��-1��ʧ�ܣ����򷵻��Ѿ�д���log����
    int log(CWX_UINT64 sid);
    ///ǿ��fsync��־�ļ���0���ɹ���-1��ʧ��
    int fsync();
public:
	///ɾ�������ļ�
	inline static void removeFile(string const& file)
	{
		string strFile = file;
		CwxFile::rmFile(strFile.c_str());
		//remove old file
		strFile = file + ".old";
		CwxFile::rmFile(strFile.c_str());
		//remove new file
		strFile = file + ".new";
		CwxFile::rmFile(strFile.c_str());
	}

    ///��ȡϵͳ�ļ�������
    inline string const& getFileName() const
    {
        return m_strFileName;
    }
    ///��ȡ������Ϣ
    inline char const* getErrMsg() const
    {
        return m_szErr2K;
    }
    ///�Ƿ���Ч
    inline bool isValid() const
    {
        return m_fd!=NULL;
    }
    inline CWX_UINT32 getCurLogCount() const
    {
        return m_uiCurLogCount;
    }
    inline CWX_UINT32 getTotalLogCount() const
    {
        return m_uiTotalLogCount;
    }
	inline CWX_UINT32 getLastSaveTime() const
	{
		return m_uiLastSaveTime;
	}
private:
    ///0���ɹ���-1��ʧ��
    int prepare();
    ///0���ɹ���-1��ʧ��
    int load( CwxMqQueueInfo& queues,
        set<CWX_UINT64>& uncommitSets,
        set<CWX_UINT64>& commitSets);
    ///0���ɹ���-1��ʧ��
    int parseQueue(string const& line, CwxMqQueueInfo& queue);
    ///0���ɹ���-1��ʧ��
    int parseSid(string const& line, CWX_UINT64& ullSid);
    ///�ر��ļ�
    inline void closeFile(bool bSync=true)
    {
        if (m_fd)
        {
            if (bSync) ::fsync(fileno(m_fd));
            if (m_bLock)
            {
                CwxFile::unlock(fileno(m_fd));
                m_bLock = false;
            }
            fclose(m_fd);
            m_fd = NULL;
        }
    }
private:
    string          m_strFileName; ///<ϵͳ�ļ�����
    string          m_strOldFileName; ///<��ϵͳ�ļ�����
    string          m_strNewFileName; ///<��ϵͳ�ļ�������
    FILE*           m_fd; ///<�ļ�handle
    bool            m_bLock; ///<�ļ��Ƿ��Ѿ�����
    CWX_UINT32      m_uiFsyncInternal; ///<flushӲ�̵ļ��
    CWX_UINT32      m_uiCurLogCount; ///<���ϴ�fsync����log��¼�Ĵ���
    CWX_UINT32      m_uiTotalLogCount; ///<��ǰ�ļ�log������
    CWX_UINT32      m_uiLine; ///<��ȡ�ļ��ĵ�ǰ����
	CWX_UINT32      m_uiLastSaveTime; ///<��һ��log�ļ��ı���ʱ��
    char            m_szErr2K[2048]; ///<������Ϣ
};

#endif 
