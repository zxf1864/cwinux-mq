#ifndef __CWX_MQ_SYS_FILE_H__
#define __CWX_MQ_SYS_FILE_H__
/*
��Ȩ������
    �����Ϊ�������У���ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
�����������⣺
    ��Ѷ��˾������Ѷ��˾��ֱ��ҵ���������ϵ�Ĺ�˾����ʹ�ô������ԭ��ɲο���
http://it.sohu.com/20100903/n274684530.shtml
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxMqMacro.h"
#include "CwxSysLogFile.h"
#include "CwxMutexLock.h"
#include "CwxStl.h"
#include "CwxStlFunc.h"

/**
@file CwxMqSysFile.h
@brief MQ��Ϣ�ַ��ķַ����¼�ļ���
@author cwinux@gmail.com
@version 1.0
@date 2010-09-23
@warning
@bug
*/

class CwxMqSysFile
{
public:
    enum
    {
        SWITCH_SYS_FILE_NUM = 100000, ///<ͬһ��ϵͳ�ļ���д���ٴ��л��ļ�
        DEF_WRITE_DISK_INTERNAL = 100 ///<������Ϣ���е�sid���ٴΣ�д�������ļ�����ֻ��write����fsync
    };
public:
    CwxMqSysFile(CWX_UINT32 uiWriteDiskInternal, string const& strFileName);
    ~CwxMqSysFile();
public:
    ///��ʼ��ϵͳ�ļ���0���ɹ���-1��ʧ��
    int init(set<string> const& queues);
public:
    ///�ύϵͳ�ļ���0���ɹ���-1��ʧ��
    inline int commit()
    {
        if (!m_bValid) return -1;
        {
            if (m_uiModifyCount)
            {
                if (!write()) return -1;
            }
            return m_pSysLogFile->commit();
        }
    }
    ///��¼�µ�sid��1���ɹ���0�����в����ڣ�-1��ʧ��
    inline int setSid(string const& strQueue, CWX_UINT64 ullSid, bool bMoreChange=true)
    {
        if (!m_bValid) return -1;
        {
            map<string, CWX_UINT64>::iterator iter=m_sids.find(strQueue);
            if (iter == m_sids.end()) return 0;
            if (bMoreChange && (iter->second>=ullSid)) return 1;
            m_uiModifyCount++;
            iter->second = ullSid;
            if (m_uiModifyCount > m_uiWriteDiskInternal)
            {
                if (!write()) return -1;
            }
        }
        return 1;
    }
    ///��ȡ��ǰ��¼��sid
    inline bool getSid(string const& strQueue, CWX_UINT64& ullSid) const
    {
        map<string, CWX_UINT64>::const_iterator iter = m_sids.find(strQueue);
        if (iter == m_sids.end()) return false;
        ullSid = iter->second;
        return true;
    }
    ///��ȡ���еĵ�ǰsid
    inline map<string, CWX_UINT64> const& getQueues() const
    {
        return m_sids;
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
        return m_bValid;
    }
    ///�Ƿ���ڶ���
    inline bool isExistQueue(string const& strQueue)
    {
        return m_sids.find(strQueue) != m_sids.end();
    }
private:
    inline bool write()
    {
        m_uiModifyCount = 0;
        map<string, CWX_UINT64>::const_iterator iter = m_sids.begin();
        if (!m_szBuf)
        {
            CWX_UINT32 uiLen = 0;
            while(iter != m_sids.end())
            {
                uiLen = iter->first.length() + 64;
                iter++;
            }
            m_szBuf = new char[uiLen];
            if (!m_szBuf)
            {
                CwxCommon::snprintf(m_szErr2K, 2047, "Failure to malloc buf, size=%u", uiLen);
                return false;
            }
            iter = m_sids.begin();
        }
        CWX_UINT32 uiPos = 0;
        char sid[32];
        while (iter != m_sids.end())
        {
            CwxCommon::toString(iter->second, sid, 16);
            uiPos += sprintf(m_szBuf + uiPos, "%s=%s\n", iter->first.c_str(), sid);
            iter++;
        }
        if (0 != m_pSysLogFile->write(m_szBuf, uiPos, true))
        {
            m_bValid = false;
            strcpy(m_szErr2K, m_pSysLogFile->getErrMsg());
            return false;
        }
        return true;
    }

private:
    string          m_strFileName; ///<ϵͳ�ļ�����
    map<string, CWX_UINT64>  m_sids; ///<mq��map
    CwxSysLogFile*  m_pSysLogFile; ///<ϵͳ�ļ�handle
    CWX_UINT32      m_uiWriteDiskInternal; ///<sid�޸Ķ��ٴ�дһ��Ӳ��
    CWX_UINT32      m_uiModifyCount; ///<����sid�޸ĵĴ���
    char            m_szErr2K[2048]; ///<������Ϣ
    bool            m_bValid;  ///<�Ƿ���Ч
    char*           m_szBuf; ///<sid�����buf��
};

#endif 
