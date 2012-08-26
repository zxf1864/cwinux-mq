#ifndef __CWX_MQ_TSS_H__
#define __CWX_MQ_TSS_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqTss.h
@brief MQϵ�з����TSS�����ļ���
@author cwinux@gmail.com
@version 0.1
@date 2010-09-15
@warning
@bug
*/

#include "CwxMqMacro.h"
#include "CwxLogger.h"
#include "CwxTss.h"
#include "CwxPackageReader.h"
#include "CwxPackageWriter.h"
#include "CwxBinLogMgr.h"

//mq��tss
class CwxMqTss:public CwxTss
{
public:
    enum
    {
        MAX_PACKAGE_SIZE = CWX_MQ_MAX_MSG_SIZE ///<�ַ����ݰ�����󳤶�
    };
public:
    ///���캯��
    CwxMqTss():CwxTss()
    {
        m_pReader = NULL;
        m_pWriter = NULL;
        m_szDataBuf = NULL;
        m_uiDataBufLen = 0;
        m_pBinlogData = NULL;
    }
    ///��������
    ~CwxMqTss();
public:
    ///tss�ĳ�ʼ����0���ɹ���-1��ʧ��
    int init();
    ///��ȡpackage��buf������NULL��ʾʧ��
    inline char* getBuf(CWX_UINT32 uiSize)
    {
        if (m_uiDataBufLen < uiSize)
        {
            delete [] m_szDataBuf;
            m_szDataBuf = new char[uiSize];
            m_uiDataBufLen = uiSize;
        }
        return m_szDataBuf;
    }
public:
    CwxPackageReader*      m_pReader; ///<���ݰ��Ľ������
    CwxPackageWriter*      m_pWriter; ///<���ݰ���pack����
    CwxPackageWriter*      m_pItemWriter; ///<chunkʱ��һ����Ϣ�����ݰ���pack����
    CwxBinLogHeader        m_header; ///<mq fetchʱ������ʧ����Ϣ��header
    CwxKeyValueItem        m_kvData; ///<mq fetchʱ������ʧ����Ϣ������
    CwxKeyValueItem const*  m_pBinlogData; ///<binlog��data������binglog�ķַ�
private:
    char*                  m_szDataBuf; ///<����buf
    CWX_UINT32             m_uiDataBufLen; ///<����buf�Ŀռ��С
};





#endif
