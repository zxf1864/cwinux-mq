#ifndef __CWX_MQ_CONFIG_H__
#define __CWX_MQ_CONFIG_H__
/*
��Ȩ������
    �����Ϊ�������У���ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
�����������⣺
    ��Ѷ��˾������Ѷ��˾��ֱ��ҵ���������ϵ�Ĺ�˾����ʹ�ô������ԭ��ɲο���
http://it.sohu.com/20100903/n274684530.shtml
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/

#include "CwxMqMacro.h"
#include "CwxGlobalMacro.h"
#include "CwxHostInfo.h"
#include "CwxCommon.h"
#include "CwxXmlFileConfigParser.h"
#include "CwxBinLogMgr.h"
#include "CwxStl.h"
#include "CwxStlFunc.h"
#include "CwxMqDef.h"

CWINUX_USING_NAMESPACE

///�����ļ���common��������
class CwxMqConfigCmn
{
public:
    enum
    {
        MIN_ASYNC_WINDOW_SIZE = 1, ///<��С���첽�ַ��Ĵ��ڴ�С
        MAX_ASYNC_WINDOW_SIZE = 1024, ///<�����첽�ַ��Ĵ��ڴ�С
        DEF_ASYNC_WINDOW_SIZE = 128 ///<ȱʡ���첽�ַ��Ĵ��ڴ�С
    };
public:
    CwxMqConfigCmn()
    {
        m_bMaster = false;
        m_uiDispatchWindowSize = DEF_ASYNC_WINDOW_SIZE;
        m_uiFromMasterWindowSize = DEF_ASYNC_WINDOW_SIZE;
    };
public:
    string              m_strWorkDir;///<����Ŀ¼
    bool                m_bMaster; ///<�Ƿ���master dispatch
    CwxHostInfo         m_mgrListen;///<�����tcp�ļ���ip/port
    CWX_UINT32          m_uiDispatchWindowSize; ///<���͵Ĵ��ڴ�С
    CWX_UINT32          m_uiFromMasterWindowSize; ///<��master���յĴ��ڴ�С
};

///�����ļ���binlog��������
class CwxMqConfigBinLog
{
public:
    enum
    {
        DEF_BINLOG_MSIZE = 2048, ///<ȱʡ��binlog��С
        MIN_BINLOG_MSIZE = 64, ///<��С��binlog��С
        MAX_BINLOG_MSIZE = 8192 ///<����binlog��С
    };
public:
    CwxMqConfigBinLog()
    {
        m_uiBinLogMSize = DEF_BINLOG_MSIZE;
        m_uiMgrMaxDay = CwxBinLogMgr::DEF_MANAGE_MAX_DAY;
        m_uiFlushNum = 100;
        m_uiFlushSecond = 30;
        m_uiMqFetchFlushNum = 1;
        m_uiMqFetchFlushSecond = 30;
    }
public:
    string              m_strBinlogPath; ///<binlog��Ŀ¼
    string              m_strBinlogPrex; ///<binlog���ļ���ǰ׺
    CWX_UINT32          m_uiBinLogMSize; ///<binlog�ļ�������С����λΪM
    CWX_UINT32          m_uiMgrMaxDay; ///<�����binglog����С����
    CWX_UINT32          m_uiFlushNum; ///<���ն�������¼��flush binlog�ļ�
    CWX_UINT32          m_uiFlushSecond; ///<��������룬����flush binlog�ļ�
    CWX_UINT32          m_uiMqFetchFlushNum; ///<fetch��������־������flush��ȡ��
    CWX_UINT32          m_uiMqFetchFlushSecond; ///<���������flush��ȡ��
};

///�����ļ���master��������
class CwxMqConfigMaster
{
public:
    CwxMqConfigMaster()
    {
    }
public:
    CwxHostInfo     m_recv; ///<master����Ϣ�ӿڶ˿���Ϣ
    CwxHostInfo     m_async; ///<master�첽�ַ��ķַ��˿���Ϣ
};

///�����ļ���slave��������
class CwxMqConfigSlave
{
public:
    CwxMqConfigSlave()
    {
    }
public:
    CwxHostInfo     m_master; ///<slave��master��������Ϣ
    string          m_strSubScribe;///<��Ϣ���ı��ʽ
    CwxHostInfo     m_async; ///<slave�첽�ַ��ķַ��˿���Ϣ
};

///�����ļ���mq��������
class CwxMqConfigMq
{
public:
    CwxMqConfigMq()
    {
    }
public:
    CwxMqConfigQueue const* getQueue(string const& strQueue) const
    {
        map<string, CwxMqConfigQueue>::const_iterator iter = m_queues.find(strQueue);
        return iter == m_queues.end()?NULL:&iter->second;
    }
public:
    CwxHostInfo     m_listen; ///<mq��listen��������Ϣ
    map<string, CwxMqConfigQueue>  m_queues; ///<��Ϣ�ַ��Ķ���
};

///�����ļ����ض���
class CwxMqConfig
{
public:
    ///���캯��
    CwxMqConfig()
    {
        m_szErrMsg[0] = 0x00;
    }
    ///��������
    ~CwxMqConfig()
    {
    }
public:
    //���������ļ�.-1:failure, 0:success
    int loadConfig(string const & strConfFile);
    //������ص������ļ���Ϣ
    void outputConfig() const;
public:
    ///��ȡcommon������Ϣ
    inline CwxMqConfigCmn const& getCommon() const
    {
        return  m_common;
    }
    ///��ȡbinlog������Ϣ
    inline CwxMqConfigBinLog const& getBinLog() const
    {
        return m_binlog;
    }
    ///��ȡmaster������Ϣ
    inline CwxMqConfigMaster const& getMaster() const
    {
        return m_master;
    }
    ///��ȡslave������Ϣ
    inline CwxMqConfigSlave const& getSlave() const 
    {
        return m_slave;
    }
    inline CwxMqConfigMq const& getMq() const
    {
        return m_mq;
    }
    ///��ȡ�����ļ����ص�ʧ��ԭ��
    inline char const* getErrMsg() const 
    {
        return m_szErrMsg;
    };
private:
    bool fetchHost(CwxXmlFileConfigParser& parser,
        string const& path,
        CwxHostInfo& host);
private:
    CwxMqConfigCmn  m_common; ///<common��������Ϣ
    CwxMqConfigBinLog m_binlog; ///<binlog��������Ϣ
    CwxMqConfigMaster m_master; ///<master��������Ϣ
    CwxMqConfigSlave  m_slave; ///<slave��������Ϣ
    CwxMqConfigMq     m_mq; ///<mq��fetch��������Ϣ
    char                m_szErrMsg[2048];///<������Ϣ��buf
};

#endif
