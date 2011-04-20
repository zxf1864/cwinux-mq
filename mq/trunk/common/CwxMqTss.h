#ifndef __CWX_MQ_TSS_H__
#define __CWX_MQ_TSS_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
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
#include "CwxAppLogger.h"
#include "CwxAppTss.h"
#include "CwxPackageReader.h"
#include "CwxPackageWriter.h"

//mq��tss
class CwxMqTss:public CwxAppTss
{
public:
    enum
    {
        MAX_PACKAGE_SIZE = 10 * 1024 * 1024 ///<�ַ����ݰ�����󳤶�
    };
public:
    ///���캯��
    CwxMqTss():CwxAppTss(new CwxAppTssInfo)
    {
        m_pReader = NULL;
        m_pWriter = NULL;
        m_szDataBuf = NULL;
        m_uiDataBufLen = 0;
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
private:
    char*                  m_szDataBuf; ///<����buf
    CWX_UINT32             m_uiDataBufLen; ///<����buf�Ŀռ��С
};





#endif
