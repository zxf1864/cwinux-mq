#ifndef __CWX_MQ_IMPORT_CONFIG_H__
#define __CWX_MQ_IMPORT_CONFIG_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxHostInfo.h"
#include "CwxCommon.h"
#include "CwxXmlFileConfigParser.h"

CWINUX_USING_NAMESPACE

///mqѹ�����Ե������ļ����ض���
class CwxMqImportConfig
{
public:
    CwxMqImportConfig(){
        m_bTcp = true;
        m_unConnNum = 0;
        m_unDataSize = 0;
        m_uiGroup = 0;
        m_uiType =0 ;
        m_bLasting = true;
    }
    
    ~CwxMqImportConfig(){}
public:
    //���������ļ�.-1:failure, 0:success
    int loadConfig(string const & strConfFile);
    //��������ļ�
    void outputConfig(string & strConfig);
    //��ȡ���������ļ���ʧ�ܴ�����Ϣ
    char const* getError() { return m_szError; };
    
public:
    bool                m_bTcp; ///<�Ƿ�ͨ��tcp��������
    string              m_strUnixPathFile;///<������unix domain���ӣ���Ϊ���ӵ�path-file
    string              m_strWorkDir;///<����Ŀ¼
    CWX_UINT16           m_unConnNum;///<���ӵ�����
    CWX_UINT16           m_unDataSize;///<���ݵĴ�С
    CWX_UINT32           m_uiGroup; ///<���ݵķ���
    CWX_UINT32           m_uiType; ///<���ݵ�����
    bool                m_bLasting;///<�Ƿ�Ϊ�־����ӣ�����HTTP��keep-alive
    CwxHostInfo       m_listen;///<tcp���ӵĶԷ�listen��ַ
    string              m_strUser; ///<fetch���û���
    string              m_strPasswd; ///<fetch���û�����
    char                m_szError[2048];///<������Ϣbuf
};

#endif
