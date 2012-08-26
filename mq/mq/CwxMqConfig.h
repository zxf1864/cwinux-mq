#ifndef __CWX_MQ_CONFIG_H__
#define __CWX_MQ_CONFIG_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/

#include "CwxMqMacro.h"
#include "CwxGlobalMacro.h"
#include "CwxHostInfo.h"
#include "CwxCommon.h"
#include "CwxIniParse.h"
#include "CwxBinLogMgr.h"
#include "CwxStl.h"
#include "CwxStlFunc.h"
#include "CwxMqDef.h"

CWINUX_USING_NAMESPACE

///�����ļ���common��������
class CwxMqConfigCmn{
public:
    enum{
        DEF_SOCK_BUF_KB = 64,
        MIN_SOCK_BUF_KB = 4,
        MAX_SOCK_BUF_KB = 8 * 1024,
        DEF_CHUNK_SIZE_KB = 32,
        MIN_CHUNK_SIZE_KB = 4,
        MAX_CHUNK_SIZE_KB = CWX_MQ_MAX_CHUNK_KSIZE,
        DEF_SYNC_CONN_NUM = 10,
        MIN_SYNC_CONN_NUM = 1,
        MAX_SYNC_CONN_NUM = 128
    };
public:
    CwxMqConfigCmn(){
        m_bMaster = false;
        m_uiSockBufSize = DEF_SOCK_BUF_KB;
        m_uiChunkSize = DEF_CHUNK_SIZE_KB;
        m_uiSyncConnNum = DEF_SYNC_CONN_NUM;
    };
public:
    string              m_strWorkDir;///<����Ŀ¼
    bool                m_bMaster; ///<�Ƿ���master dispatch
    CWX_UINT32          m_uiSockBufSize; ///<�ַ���socket���ӵ�buf��С
    CWX_UINT32          m_uiChunkSize; ///<Trunk�Ĵ�С
    CWX_UINT32          m_uiSyncConnNum;   ///<ͬ����������
    CwxHostInfo         m_monitor; ///<��ؼ���
};

///�����ļ���binlog��������
class CwxMqConfigBinLog{
public:
    enum{
        DEF_BINLOG_MSIZE = 1024, ///<ȱʡ��binlog��С
        MIN_BINLOG_MSIZE = 64, ///<��С��binlog��С
        MAX_BINLOG_MSIZE = 2048 ///<����binlog��С
    };
public:
    CwxMqConfigBinLog(){
        m_uiBinLogMSize = DEF_BINLOG_MSIZE;
        m_uiMgrFileNum = CwxBinLogMgr::DEF_MANAGE_FILE_NUM;
        m_bDelOutdayLogFile = false;
        m_uiFlushNum = 100;
        m_uiFlushSecond = 30;
    }
public:
    string              m_strBinlogPath; ///<binlog��Ŀ¼
    string              m_strBinlogPrex; ///<binlog���ļ���ǰ׺
    CWX_UINT32          m_uiBinLogMSize; ///<binlog�ļ�������С����λΪM
    CWX_UINT32          m_uiMgrFileNum; ///<�����binglog������ļ���
    bool                m_bDelOutdayLogFile; ///<�Ƿ�ɾ�����������Ϣ�ļ�
    CWX_UINT32          m_uiFlushNum; ///<���ն�������¼��flush binlog�ļ�
    CWX_UINT32          m_uiFlushSecond; ///<��������룬����flush binlog�ļ�
};

///�����ļ���master��������
class CwxMqConfigMaster{
public:
    CwxMqConfigMaster(){
    }
public:
    CwxHostInfo     m_recv; ///<master��binЭ��˿���Ϣ
    CwxHostInfo     m_async; ///<master binЭ���첽�ַ��˿���Ϣ
};

///�����ļ���slave��������
class CwxMqConfigSlave{
public:
    CwxMqConfigSlave(){
        m_bzip = false;
    }
public:
    CwxHostInfo     m_master; ///<slave��master��������Ϣ
    string          m_strSubScribe;///<��Ϣ���ı��ʽ
    bool            m_bzip; ///<�Ƿ�zipѹ��
    string          m_strSign; ///<ǩ������
    CwxHostInfo     m_async; ///<slave binЭ���첽�ַ��Ķ˿���Ϣ
};

///�����ļ���mq����
class CwxMqConfigMq{
public:
    CwxMqConfigMq(){
        m_uiFlushNum = 1;
        m_uiFlushSecond = 30;
    }
public:
    CwxHostInfo          m_mq; ///<mq��fetch��������Ϣ
    string               m_strLogFilePath; ///<mq��log�ļ���Ŀ¼
    CWX_UINT32          m_uiFlushNum; ///<fetch��������־������flush��ȡ��
    CWX_UINT32          m_uiFlushSecond; ///<���������flush��ȡ��

};

///�����ļ����ض���
class CwxMqConfig{
public:
    ///���캯��
    CwxMqConfig(){
        m_szErrMsg[0] = 0x00;
    }
    ///��������
    ~CwxMqConfig(){
    }
public:
    //���������ļ�.-1:failure, 0:success
    int loadConfig(string const & strConfFile);
    //������ص������ļ���Ϣ
    void outputConfig() const;
public:
    ///��ȡcommon������Ϣ
    inline CwxMqConfigCmn const& getCommon() const{
        return  m_common;
    }
    ///��ȡbinlog������Ϣ
    inline CwxMqConfigBinLog const& getBinLog() const{
        return m_binlog;
    }
    ///��ȡmaster������Ϣ
    inline CwxMqConfigMaster const& getMaster() const{
        return m_master;
    }
    ///��ȡslave������Ϣ
    inline CwxMqConfigSlave const& getSlave() const {
        return m_slave;
    }
    inline CwxMqConfigMq const& getMq() const{
        return m_mq;
    }
    ///��ȡ�����ļ����ص�ʧ��ԭ��
    inline char const* getErrMsg() const {
        return m_szErrMsg;
    };
private:
    bool fetchHost(CwxIniParse& cnf,
        string const& node,
        CwxHostInfo& host);
private:
    CwxMqConfigCmn      m_common; ///<common��������Ϣ
    CwxMqConfigBinLog   m_binlog; ///<binlog��������Ϣ
    CwxMqConfigMaster   m_master; ///<master��������Ϣ
    CwxMqConfigSlave    m_slave; ///<slave��������Ϣ
    CwxMqConfigMq       m_mq; ///<mq��fetch��������Ϣ
    char                m_szErrMsg[2048];///<������Ϣ��buf
};

#endif
