#ifndef __CWX_MQ_POCO_H__
#define __CWX_MQ_POCO_H__
/*
��Ȩ������
    �������ѭGNU LGPL��http://www.gnu.org/copyleft/lesser.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
/**
@file CwxMqPoco.h
@brief MQϵ�з���Ľӿ�Э�鶨�����
@author cwinux@gmail.com
@version 1.0
@date 2010-09-23
@warning
@bug
*/

#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxMsgBlock.h"

class CwxMqSubscribeItem
{
public:
    CwxMqSubscribeItem()
    {
        m_bAll = false;
        m_bMod = false;
        m_uiModBase = 0;
        m_uiModIndex = 0;
    }
    CwxMqSubscribeItem(CwxMqSubscribeItem const& item)
    {
        m_bAll = item.m_bAll;
        m_bMod = item.m_bMod;
        m_uiModBase = item.m_uiModBase;
        m_uiModIndex = item.m_uiModIndex;
        m_set = item.m_set;
    }
    CwxMqSubscribeItem& operator=(CwxMqSubscribeItem const& item)
    {
        if (this != &item)
        {
            m_bAll = item.m_bAll;
            m_bMod = item.m_bMod;
            m_uiModBase = item.m_uiModBase;
            m_uiModIndex = item.m_uiModIndex;
            m_set = item.m_set;
        }
        return *this;
    }
public:
    inline bool isSubscribe(CWX_UINT32 id) const
    {
        if (m_bAll) return true;
        if (m_bMod) return (id%m_uiModBase)==m_uiModIndex;
        list<pair<CWX_UINT32, CWX_UINT32> >::const_iterator iter = m_set.begin();
        while(iter != m_set.end())
        {
            if ((id>=iter->first) && (id<=iter->second)) return true;
            iter++;
        }
        return false;
    }
public:
    bool    m_bAll; ///<�Ƿ�ȫ������
    bool    m_bMod; ///<�Ƿ�Ϊ����ģʽ
    CWX_UINT32  m_uiModBase; ///<����Ļ���
    CWX_UINT32  m_uiModIndex; ///<����ֵ
    list<pair<CWX_UINT32, CWX_UINT32> > m_set; ///<���ĵ�group��type�ķ�Χ�б�
};

class CwxMqSubscribe
{
public:
    CwxMqSubscribe()
    {
        m_bAll = false;
    }

    CwxMqSubscribe(CwxMqSubscribe const& item)
    {
        m_bAll = item.m_bAll;
        m_subscribe = item.m_subscribe;
    }

    CwxMqSubscribe& operator=(CwxMqSubscribe const& item)
    {
        if (this != &item)
        {
            m_bAll = item.m_bAll;
            m_subscribe = item.m_subscribe;
        }
        return *this;
    }

public:
    inline bool isSubscribe(CWX_UINT32 uiGroup, CWX_UINT32 uiType) const
    {
        if (!m_bAll)
        {
            list<pair<CwxMqSubscribeItem/*group*/, CwxMqSubscribeItem/*type*/> >::const_iterator iter = m_subscribe.begin();
            while(iter != m_subscribe.end())
            {
                if (iter->first.isSubscribe(uiGroup) && iter->second.isSubscribe(uiType)) return true;
                iter++;
            }
            return false;
        }
        return true;
    }
public:
    bool    m_bAll; ///<�Ƿ���ȫ����Ϣ
    list<pair<CwxMqSubscribeItem/*group*/, CwxMqSubscribeItem/*type*/> > m_subscribe; ///<���Ĺ����б�
};

//mq��Э�鶨�����
class CwxMqPoco
{
public:
    enum ///<��Ϣ���Ͷ���
    {
        ///RECV�������͵���Ϣ���Ͷ���
        MSG_TYPE_RECV_DATA = 1, ///<�����ύ��Ϣ
        MSG_TYPE_RECV_DATA_REPLY = 2, ///<�����ύ��Ϣ�Ļظ�
        MSG_TYPE_RECV_COMMIT = 3, ///<����commit��Ϣ
        MSG_TYPE_RECV_COMMIT_REPLY = 4, ///<commit��Ϣ�Ļظ�
        ///�ַ�����Ϣ���Ͷ���
        MSG_TYPE_SYNC_REPORT = 5, ///<ͬ��SID�㱨����Ϣ����
        MSG_TYPE_SYNC_REPORT_REPLY = 6, ///<ʧ�ܷ���
        MSG_TYPE_SYNC_DATA = 7,  ///<��������
        MSG_TYPE_SYNC_DATA_REPLY = 8, ///<���ݵĻظ�
        ///MQ Fetch�������͵���Ϣ���Ͷ���
        MSG_TYPE_FETCH_DATA = 9, ///<���ݻ�ȡ��Ϣ����
        MSG_TYPE_FETCH_DATA_REPLY = 10, ///<�ظ����ݻ�ȡ��Ϣ����
    };
    enum
    {
        SYNC_GROUP_TYPE=0XFFFFFFFF,
        SYNC_SECOND_INTERNAL=60,
        SYNC_RECORD_INTERNAL=10000,
        MAX_CONTINUE_SEEK_NUM = 2048
    };
public:
    ///��ʼ��Э�顣����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int init(char* szErr2K=NULL);
    ///�ͷ�Э�顣
    static void destory();
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packRecvData(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        CwxKeyValueItem const& data,
        CWX_UINT32 group,
        CWX_UINT32 type,
        CWX_UINT32 attr,
        char const* user=NULL,
        char const* passwd=NULL,
        char* szErr2K=NULL
        );
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseRecvData(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        CwxKeyValueItem const*& data,
        CWX_UINT32& group,
        CWX_UINT32& type,
        CWX_UINT32& attr,
        char const*& user,
        char const*& passwd,
        char* szErr2K=NULL);


    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packRecvDataReply(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        int ret,
        CWX_UINT64 ullSid,
        char const* szErrMsg,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseRecvDataReply(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        int& ret,
        CWX_UINT64& ullSid,
        char const*& szErrMsg,
        char* szErr2K=NULL);

    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packCommit(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        char const* user=NULL,
        char const* passwd=NULL,
        char* szErr2K=NULL
        );
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseCommit(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        char const*& user,
        char const*& passwd,
        char* szErr2K=NULL);



    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packCommitReply(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        int ret,
        char const* szErrMsg,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseCommitReply(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        int& ret,
        char const*& szErrMsg,
        char* szErr2K=NULL);


    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packReportData(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        CWX_UINT64 ullSid,
        bool      bNewly,
        char const* subscribe = NULL,
        char const* user=NULL,
        char const* passwd=NULL,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseReportData(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        CWX_UINT64& ullSid,
        bool&       bNewly,
        char const*& subscribe,
        char const*& user,
        char const*& passwd,
        char* szErr2K=NULL);


    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packReportDataReply(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        int ret,
        CWX_UINT64 ullSid,
        char const* szErrMsg,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseReportDataReply(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        int& ret,
        CWX_UINT64& ullSid,
        char const*& szErrMsg,
        char* szErr2K=NULL);

    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packSyncData(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        CWX_UINT64 ullSid,
        CWX_UINT32 uiTimeStamp,
        CwxKeyValueItem const& data,
        CWX_UINT32 group,
        CWX_UINT32 type,
        CWX_UINT32 attr,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseSyncData(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        CWX_UINT64& ullSid,
        CWX_UINT32& uiTimeStamp,
        CwxKeyValueItem const*& data,
        CWX_UINT32& group,
        CWX_UINT32& type,
        CWX_UINT32& attr,
        char* szErr2K=NULL);

    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packSyncDataReply(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        CWX_UINT32 uiTaskId,
        CWX_UINT64 ullSid,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseSyncDataReply(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        CWX_UINT64& ullSid,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packFetchMq(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        bool bBlock,
        char const* queue_name,
        char const* user=NULL,
        char const* passwd=NULL,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseFetchMq(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        bool& bBlock,
        char const*& queue_name,
        char const*& user,
        char const*& passwd,
        char* szErr2K=NULL);

    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int packFetchMqReply(CwxPackageWriter* writer,
        CwxMsgBlock*& msg,
        int  ret,
        char const* szErrMsg,
        CWX_UINT64 ullSid,
        CWX_UINT32 uiTimeStamp,
        CwxKeyValueItem const& data,
        CWX_UINT32 group,
        CWX_UINT32 type,
        CWX_UINT32 attr,
        char* szErr2K=NULL);
    ///����ֵ��CWX_MQ_SUCCESS���ɹ�����������ʧ��
    static int parseFetchMqReply(CwxPackageReader* reader,
        CwxMsgBlock const* msg,
        int&  ret,
        char const*& szErrMsg,
        CWX_UINT64& ullSid,
        CWX_UINT32& uiTimeStamp,
        CwxKeyValueItem const* data,
        CWX_UINT32& group,
        CWX_UINT32& type,
        CWX_UINT32& attr,
        char* szErr2K=NULL);
    ///true����Ҫ��Ʒsync��¼��false������Ҫ����sync��¼
    inline static bool isNeedSyncRecord(CWX_UINT32 uiRecordNum, time_t ttLastSyncTime)
    {
        return (uiRecordNum>SYNC_RECORD_INTERNAL) || ((CWX_UINT32)(time(NULL) - ttLastSyncTime) > SYNC_SECOND_INTERNAL);
    }
    ///true����sync��¼��false������sync��¼
    inline static bool isSyncRecord(CWX_UINT32 uiGroup)
    {
        return uiGroup == SYNC_GROUP_TYPE;
    }
    ///����sync��¼��
    inline static char const* getSyncRecordData()
    {
        return m_pWriter->getMsg();
    }
    ///��ȡsync��¼�ĳ���
    inline static CWX_UINT32 getSyncRecordDataLen()
    {
        return m_pWriter->getMsgSize();
    }
    ///�Ƿ�������Ҷ��ĵ���Ϣ����
    inline static bool isContinueSeek(CWX_UINT32 uiSeekedNum)
    {
        return MAX_CONTINUE_SEEK_NUM>uiSeekedNum;
    }
    ///�Ƿ�Ϊ��Ч����Ϣ�����﷨
    static bool isValidSubscribe(string const& strSubscribe, string& strErrMsg);
    ///�������ĵ��﷨
    /*
    ���ʽΪ
    group_express:type_express;group_express:type_express...
    ���У�
    group_express: [*]|[group_index%group_num]|[begin-end,begin-end,...]
    *��ȫ��
    group_index%group_num����group��group_num���࣬����Ϊgroup_index�ġ�
    begin-end��group��Χ�������Χ�����ԡ�,���ָ��begin==end����ֻдbegin�Ϳ�����
    type_express:  [*]|[type_index%typte_num]|[begin-end,begin-end,...]
    �����ֺ���ͬgroup
    */
    static bool parseSubsribe(string const& strSubscribe, CwxMqSubscribe& subscribe, string& strErrMsg);
    ///��Ϣ�Ƿ���
    inline static bool isSubscribe(CwxMqSubscribe const& subscribe, bool bSync, CWX_UINT32 uiGroup, CWX_UINT32 uiType)
    {
        if (uiGroup == SYNC_GROUP_TYPE)
        {
            return bSync;
        }
        return subscribe.isSubscribe(uiGroup, uiType);
    }
private:
    ///��ֹ��������ʵ��
    CwxMqPoco()
    {
    }
    ///��������
    ~CwxMqPoco();
    ///����һ�����Ĺ���
    static bool parseSubsribeRule(string const& strSubsribeRule,
        pair<CwxMqSubscribeItem/*group*/, CwxMqSubscribeItem/*type*/>& rule,
        bool& bAll,
        string& strErrMsg);
    ///����һ�����ı��ʽ
    static bool parseSubsribeExpress(string const& strSubsribeExpress,
        CwxMqSubscribeItem& express,
        string& strErrMsg);
private:
    static CwxPackageWriter*   m_pWriter;
};





#endif
