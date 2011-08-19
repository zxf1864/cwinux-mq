#ifndef __CWX_MQ_PROXY_CONFIG_H__
#define __CWX_MQ_PROXY_CONFIG_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/


#include "CwxHostInfo.h"
#include "CwxCommon.h"
#include "CwxXmlFileConfigParser.h"
#include "CwxMqDef.h"

CWINUX_USING_NAMESPACE

///echoѹ�����Ե������ļ����ض���
class CwxMproxyConfig
{
public:
    CwxMproxyConfig()
    {
        m_uiTimeout = 5000;
        m_szErrMsg[0] = 0x00;
    }
    
    ~CwxMproxyConfig()
    {
    }
public:
    //���������ļ�.-1:failure, 0:success
    int loadConfig(string const & strConfFile);
    //��������ļ�
    void outputConfig();
    //��ȡ���������ļ���ʧ�ܴ�����Ϣ
    char const* getError() { return m_szErrMsg; };
private:
    bool fetchHost(CwxXmlFileConfigParser& parser,
        string const& path,
        CwxHostInfo& host,
		bool bIpOnly=false);
    bool loadGroup(string const& path, CwxXmlTreeNode const* pGroup, CwxMqConfigQueue& group);
    bool parseIds(string const& group, list<pair<CWX_UINT32, CWX_UINT32> >& ids);

public:
    string               m_strWorkDir;///<����Ŀ¼
    CWX_UINT32           m_uiTimeout; ///<��ѯ��ʱʱ�䣬��λΪms
    CwxHostInfo          m_monitor; ///<����ļ�ص�ַ
    CwxHostInfo          m_recv;      ///<������Ϣ���ܵļ�����ַ
    map<CwxMqIdRange, CwxMqConfigQueue>  m_groupPasswd; ///<����
    map<CwxMqIdRange, string>    m_allowGroup; ///<�����group������Ϊ�գ���group������allow�д��ڣ������deny
    map<CwxMqIdRange, string>    m_denyGroup; ///<��ֹ��group����allowΪ�գ����deny������deny�д��ڣ����ֹ��
    CwxHostInfo          m_mq; ///<mq�ķ�����
    bool                 m_bzip; ///<���͸�mq����Ϣ�Ƿ�ѹ��
    string               m_mqSign; ///<mq��ǩ������
    char                 m_szErrMsg[2048];///<������Ϣbuf
};

#endif
