#ifndef __CWX_SYS_LOG_FILE_H__
#define __CWX_SYS_LOG_FILE_H__

#include "CwxMqMacro.h"
#include "CwxFile.h"
#include "CwxCommon.h"


/**
@file CwxSysLogFile.h
@brief ϵͳ��¼�ļ��������ȷ��ϵͳ�ļ���һ�����޸ġ�
@author cwinux@gmail.com
@version 1.0
@date 2010-09-23
@warning
@bug
*/

class CwxSysLogFile
{
public:
    enum
    {
        DEF_SWITCH_SYS_FILE_NUM = 100000 ///<ͬһ��ϵͳ�ļ���д���ٴ��л��ļ�
    };
public:
    CwxSysLogFile(char const* szFileName, CWX_UINT32 uiSwithFileNum=DEF_SWITCH_SYS_FILE_NUM);
    ~CwxSysLogFile();
public:
    ///��ʼ��ϵͳ�ļ���0���ɹ���-1��ʧ��
    int init();
public:
    ///�ύϵͳ�ļ���0���ɹ���-1��ʧ��
    inline int commit()
    {
        if (-1 != m_fd)
        {
            if (!m_bSaved)
            {
                if (0 != saveFile())
                {
                    ::close(m_fd);
                    m_fd = -1;
                    return -1;
                }
            }
            if (0 != ::fsync(m_fd))
            {
                CwxCommon::snprintf(m_szErrMsg, 2047, "Failure to commit sys file:%s, errno=%d",
                    m_strFileName.c_str(),
                    errno);
                ::close(m_fd);
                m_fd = -1;
                return -1;
            }
            return 0;
        }
        return -1;
    }
    ///����ϵͳ�ļ�
    inline int write(char const* szContent, CWX_UINT32 uiSize, bool bSaveFile=true)
    {
        if (-1 != m_fd)
        {
            if (!prepareBuf(uiSize))
            {
                CwxCommon::snprintf(m_szErrMsg, 2047, "Failure to alloc buf, size=%u", uiSize);
                ::close(m_fd);
                m_fd = -1;
                return -1;
            }
            memcpy(m_szFileContentBuf, szContent, uiSize);
            m_uiContentSize = uiSize;
            m_szFileContentBuf[uiSize] = 0x00;
            m_bSaved = false;
            if (bSaveFile)
            {
                if (0 != saveFile())
                {
                    ::close(m_fd);
                    m_fd = -1;
                    return -1;
                }
            }
            return 0;
        }
        return -1;
    }
    inline char const* getFileContents() const
    {
        return m_szFileContentBuf;
    }
    ///��ȡcontent�Ĵ�С
    inline CWX_UINT32 getFileContentSize() const
    {
        return m_uiContentSize;
    }
    ///��ȡϵͳ�ļ�������
    inline string const& getFileName() const
    {
        return m_strFileName;
    }
    ///��ȡ������Ϣ
    inline char const* getErrMsg() const
    {
        return m_szErrMsg;
    }
    ///�Ƿ���Ч
    inline bool isValid() const
    {
        return -1 != m_fd;
    }
private:
    ///�л�ϵͳ�ļ���0���ɹ���-1��ʧ��
    int switchSysFile();
    ///����ǰ�����ݱ��浽�ļ���0���ɹ���-1��ʧ��
    int saveFile();
    ///��ȡָ����С���ڴ棻true���ɹ���false��ʧ��
    bool prepareBuf(CWX_UINT32 uiSize);
private:
    string          m_strFileName; ///<ϵͳ�ļ�����
    string          m_strOldFileName; ///<��ϵͳ�ļ�����
    string          m_strNewFileName; ///<��ϵͳ�ļ�������
    char*           m_szFileContentBuf; ///<�ļ����ݵ�buf
    CWX_UINT32      m_uiBufLen;  ///<�ļ�����buf�ĳ���
    CWX_UINT32      m_uiFileSize; ///<��ǰ�ļ����ֽ���
    CWX_UINT32      m_uiContentSize; ///<��ǰbuf�����ݵ��ֽ���
    CWX_UINT32      m_uiSwitchLogFileCount; ///<�л�ϵͳ�ļ���д����
    int             m_fd; ///<��ǰ�ļ�handle����Ϊ-1��ʾ��Ч
    bool            m_bSaved; ///<��ǰ�������Ƿ񱣴浽�ļ�
    CWX_UINT32      m_uiWriteCount; ///<��ǰ�ļ�д�Ĵ���
    char            m_szErrMsg[2048];
};


#endif
