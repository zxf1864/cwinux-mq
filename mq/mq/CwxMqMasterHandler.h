#ifndef __CWX_MQ_MASTER_HANDLER_H__
#define __CWX_MQ_MASTER_HANDLER_H__
/*
��Ȩ������
�������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxMqMacro.h"
#include "CwxPackageReader.h"
#include "CwxPackageWriter.h"
#include "CwxMsgBlock.h"
#include "CwxMqTss.h"

class CwxMqApp;

///binlogͬ����session
class CwxMqSyncSession{
public:
    ///���캯��
    CwxMqSyncSession(CWX_UINT32 uiHostId){
        m_ullSessionId = 0;
        m_ullNextSeq = 0;
        m_uiReportDatetime = 0;
        m_uiHostId = uiHostId;
    }
    ~CwxMqSyncSession(){
        map<CWX_UINT64/*seq*/, CwxMsgBlock*>::iterator iter =  m_msg.begin();
        while(iter != m_msg.end()){
            CwxMsgBlockAlloc::free(iter->second);
            iter++;
        }
    }
public:
    ///��������Ϣ�������Ѿ��յ�����Ϣ�б�
    bool recv(CWX_UINT64 ullSeq, CwxMsgBlock* msg, list<CwxMsgBlock*>& finished){
        map<CWX_UINT32,  bool>::iterator iter = m_conns.find(msg->event().getConnId());
        if ( (iter == m_conns.end()) || !iter->second ) return false;
        finished.clear();
        if (ullSeq == m_ullNextSeq){
            finished.push_back(msg);
            m_ullNextSeq++;
            map<CWX_UINT64/*seq*/, CwxMsgBlock* >::iterator iter =  m_msg.begin();
            while(iter != m_msg.end()){
                if (iter->first == m_ullNextSeq){
                    finished.push_back(iter->second);
                    m_ullNextSeq++;
                    m_msg.erase(iter);
                    iter = m_msg.begin();
                    continue;
                }
                break;
            }
            return true;
        }
        m_msg[ullSeq] = msg;
        msg->event().setTimestamp((CWX_UINT32)time(NULL));
        return true;
    }

    //����Ƿ�ʱ
    bool isTimeout(CWX_UINT32 uiTimeout) const{
        if (!m_msg.size()) return false;
        CWX_UINT32 uiNow = time(NULL);
        return m_msg.begin()->second->event().getTimestamp() + uiTimeout < uiNow;
    }
public:
    CWX_UINT64              m_ullSessionId; ///<session id
    CWX_UINT64              m_ullNextSeq; ///<��һ�������յ�sid
    CWX_UINT32              m_uiHostId; ///<host id
    map<CWX_UINT64/*seq*/, CwxMsgBlock*>  m_msg;   ///<�ȴ��������Ϣ
    map<CWX_UINT32,  bool/*�Ƿ��Ѿ�report*/>  m_conns; ///<����������
    CWX_UINT32              m_uiReportDatetime; ///<�����ʱ�����������ָ����ʱ��û�лظ�����ر�
};

///slave��master����binlog�Ĵ���handle
class CwxMqMasterHandler : public CwxCmdOp
{
public:
    enum
    {
        RECONN_MASTER_DELAY_SECOND = 4
    };
public:
    ///���캯��
    CwxMqMasterHandler(CwxMqApp* pApp):m_pApp(pApp){
        m_unzipBuf = NULL;
        m_uiBufLen = 0;
        m_uiCurHostId = 0;
        m_syncSession = NULL;
    }
    ///��������
    virtual ~CwxMqMasterHandler(){
        if (m_unzipBuf) delete [] m_unzipBuf;
    }
public:
    ///master�����ӹرպ���Ҫ������
    virtual int onConnClosed(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��������master����Ϣ
    virtual int onRecvMsg(CwxMsgBlock*& msg, CwxTss* pThrEnv);
    ///��ʱ���
    virtual int onTimeoutCheck(CwxMsgBlock*& msg, CwxTss* pThrEnv);
public:
    ///��ȡsession
    CwxMqSyncSession*  getSession(){
        return m_syncSession; ///<����ͬ����session
    }

private:
    //�ر���������
    void closeSession();
    ///������masterͬ�������ӡ�����ֵ��0���ɹ���-1��ʧ��
    int createSession(CwxMqTss* pTss); ///<tss����
    ///�յ�һ����Ϣ�Ĵ�����������ֵ��0:�ɹ���-1��ʧ��
    int recvMsg(CwxMsgBlock*& msg, ///<�յ�����Ϣ
        list<CwxMsgBlock*>& msgs ///<���ճ��з��صĿɴ������Ϣ����list�����Ⱥ��������
        );

    ///����Sync report��reply��Ϣ������ֵ��0���ɹ���-1��ʧ��
    int dealSyncReportReply(CwxMsgBlock*& msg, ///<�յ�����Ϣ
        CwxMqTss* pTss ///<tss����
        );

    ///�����յ���sync data������ֵ��0���ɹ���-1��ʧ��
    int dealSyncData(CwxMsgBlock*& msg, ///<�յ�����Ϣ
        CwxMqTss* pTss ///<tss����
        );

    //�����յ���chunkģʽ�µ�sync data������ֵ��0���ɹ���-1��ʧ��
    int dealSyncChunkData(CwxMsgBlock*& msg, ///<�յ�����Ϣ
        CwxMqTss* pTss ///<tss����
        );

    //���������Ϣ������ֵ��0���ɹ���-1��ʧ��
    int dealErrMsg(CwxMsgBlock*& msg,  ///<�յ�����Ϣ
        CwxMqTss* pTss ///<tss����
        );

    //0���ɹ���-1��ʧ��
    int saveBinlog(CwxMqTss* pTss,
        char const* szBinLog,
        CWX_UINT32 uiLen);
    bool checkSign(char const* data,
        CWX_UINT32 uiDateLen,
        char const* szSign,
        char const* sign);
    //��ȡunzip��buf
    bool prepareUnzipBuf();
private:
    CwxMqApp*                m_pApp;  ///<app����
    CwxPackageReader         m_reader; ///<�����reader
    unsigned char*           m_unzipBuf; ///<��ѹ��buffer
    CWX_UINT32               m_uiBufLen; ///<��ѹbuffer�Ĵ�С����Ϊtrunk��20������СΪ20M��
    CwxMqSyncSession*        m_syncSession; ///<����ͬ����session
    CWX_UINT32               m_uiCurHostId; ///<��ǰ��host id
};

#endif 
