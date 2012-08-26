#ifndef __CWX_MQ_BIN_ASYNC_HANDLER_H__
#define __CWX_MQ_BIN_ASYNC_HANDLER_H__
/*
��Ȩ������
    �������ѭGNU GPL V3��http://www.gnu.org/licenses/gpl.html����
    ��ϵ��ʽ��email:cwinux@gmail.com��΢��:http://t.sina.com.cn/cwinux
*/
#include "CwxCommander.h"
#include "CwxAppAioWindow.h"
#include "CwxMqMacro.h"
#include "CwxMqTss.h"
#include "CwxMqDef.h"
#include "CwxAppHandler4Channel.h"
#include "CwxAppChannel.h"

class CwxMqApp;
class CwxMqBinAsyncHandler;

///�ַ����ӵ�sync session��Ϣ����
class CwxMqBinAsyncHandlerSession{
public:
    ///���캯��
    CwxMqBinAsyncHandlerSession(){
        m_ullSeq = 0;
        m_ullSessionId = 0;
        m_bClosed = false;
        m_pCursor = NULL;
        m_uiChunk = 0;
        m_ullStartSid = 0;
        m_ullSid = 0;
        m_bNext = false;
        m_bZip = false;
    }
    ~CwxMqBinAsyncHandlerSession(){
    }
public:
    void addConn(CwxMqBinAsyncHandler* conn);
    ///�����γ�session id������session id
    CWX_UINT64 reformSessionId(){
        CwxTimeValue timer;
        timer.now();
        m_ullSessionId = timer.to_usec();
        return m_ullSessionId;
    }

public:
    CWX_UINT64               m_ullSessionId; ///<session id
    CWX_UINT64                m_ullSeq; ///<��ǰ�����кţ���0��ʼ��
    bool                     m_bClosed; ///<�Ƿ���Ҫ�ر�
    map<CWX_UINT32, CwxMqBinAsyncHandler*> m_conns; ///<����������
    CwxBinLogCursor*         m_pCursor; ///<binlog�Ķ�ȡcursor
    CWX_UINT32               m_uiChunk; ///<chunk��С
    CWX_UINT64               m_ullStartSid; ///<report��sid
    CWX_UINT64               m_ullSid; ///<��ǰ���͵���sid
    bool                     m_bNext; ///<�Ƿ�����һ����Ϣ
    CwxMqSubscribe         m_subscribe; ///<��Ϣ���Ķ���
    string                   m_strSign; ///<ǩ������
    bool                     m_bZip; ///<�Ƿ�ѹ��
    string                   m_strHost; ///<session����Դ����
};

///�첽binlog�ַ�����Ϣ����handler
class CwxMqBinAsyncHandler : public CwxAppHandler4Channel{
public:
    ///���캯��
    CwxMqBinAsyncHandler(CwxMqApp* pApp,
        CwxAppChannel* channel,
        CWX_UINT32 uiConnId);
    ///��������
    virtual ~CwxMqBinAsyncHandler();
public:
    /**
    @brief ���ӿɶ��¼�������-1��close()�ᱻ����
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onInput();
    /**
    @brief ֪ͨ���ӹرա�
    @return 1������engine���Ƴ�ע�᣻0����engine���Ƴ�ע�ᵫ��ɾ��handler��-1����engine�н�handle�Ƴ���ɾ����
    */
    virtual int onConnClosed();
    /**
    @brief Handler��redo�¼�����ÿ��dispatchʱִ�С�
    @return -1������ʧ�ܣ������close()�� 0������ɹ�
    */
    virtual int onRedo();

public:
    ///����binlog������ֵ��0��δ����һ��binlog��1��������һ��binlog��-1��ʧ�ܣ�
    int syncSendBinLog(CwxMqTss* pTss);

    ///packһ��binlog������ֵ��-1��ʧ�ܣ�1���ɹ�
    int syncPackOneBinLog(CwxPackageWriterEx* writer, ///<writer����
        CwxMsgBlock*& block, ///<pack���γɵ����ݰ�
        CWX_UINT64 ullSeq, ///<��Ϣ���к�
        CwxKeyValueItem const* pData, ///<���������
        char* szErr2K ///<��ʧ�ܷ��ش�����Ϣ
        );

    ///pack����binlog������ֵ��-1��ʧ�ܣ�1���ɹ�
    int syncPackMultiBinLog(CwxPackageWriterEx* writer, ///<writer����
        CwxPackageWriterEx* writer_item, ///<writer����
        CwxKeyValueItem const* pData, ///<���������
        CWX_UINT32&  uiLen, ///<����pack�굱ǰbinlog���������ݰ��Ĵ�С
        char* szErr2K ///<��ʧ�ܷ��ش�����Ϣ
        );

    ///��λ����Ҫ��binlog��������ֵ��1�����ּ�¼��0��û�з��֣�-1������
    int syncSeekToBinlog(CwxMqTss* tss, ///<�߳�tss
        CWX_UINT32& uiSkipNum ///<�����Ա�����binlog����������ʣ��ֵ
        );

    ///��binlog��λ��report��sid������ֵ��1���ɹ���0��̫��-1������
    int syncSeekToReportSid(CwxMqTss* tss);

    ///����export�����ݡ�����ֵ��0��δ����һ�����ݣ�1��������һ�����ݣ�-1��ʧ�ܣ�
    int exportSendData(CwxMqTss* pTss);

    ///��ȡ����id
    inline CWX_UINT32 getConnId() const{
        return m_uiConnId;
    }

public:
    ///�ַ��̵߳��¼����ȴ�����
    static void doEvent(CwxMqApp* app, ///<app����
        CwxMqTss* tss, ///<�߳�tss
        CwxMsgBlock*& msg ///<�¼���Ϣ
        );

    ///����رյ�session
    static void dealClosedSession(CwxMqApp* app, ///<app����
        CwxMqTss* tss  ///<�߳�tss
        );
    ///�ͷ���Դ
    static void destroy(CwxMqApp* app){
        map<CWX_UINT64, CwxMqBinAsyncHandlerSession* >::iterator iter = m_sessionMap.begin();
        while(iter != m_sessionMap.end()){
            if (iter->second->m_pCursor) pApp->getStore()->getBinLogMgr()->destoryCurser(iter->second->m_pCursor);
            delete iter->second;
            iter++;
        }
        m_sessionMap.clear();
    }
private:
    ///�յ�һ����Ϣ����������ֵ��0���ɹ���-1��ʧ��
    int recvMessage();

    ///�յ�sync report����Ϣ������ֵ��0���ɹ���-1��ʧ��
    int recvSyncReport(CwxMqTss* pTss);

    ///�յ�sync new conn��report��Ϣ������ֵ��0���ɹ���-1��ʧ��
    int recvSyncNewConnection(CwxMqTss* pTss);

    ///�յ�binlog sync��reply��Ϣ������ֵ��0���ɹ���-1��ʧ��
    int recvSyncReply(CwxMqTss* pTss);

    ///�յ�chunkģʽ�µ�binlog sync reply��Ϣ������ֵ��0���ɹ���-1��ʧ��
    int recvSyncChunkReply(CwxMqTss* pTss);
private:
    bool                        m_bReport; ///<�Ƿ��Ѿ�����
    CwxMqBinAsyncHandlerSession*  m_syncSession; ///<���Ӷ�Ӧ��session
    CWX_UINT64                   m_ullSessionId; ///<session��id
    CWX_UINT64                   m_ullSentSeq; ///<���͵����к�
    CWX_UINT32                   m_uiConnId; ///<����id
    CwxMqApp*                    m_pApp;  ///<app����
    CwxMsgHead                   m_header; ///<��Ϣͷ
    char                         m_szHeadBuf[CwxMsgHead::MSG_HEAD_LEN+1]; ///<��Ϣͷ��buf
    CWX_UINT32                    m_uiRecvHeadLen; ///<recieved msg header's byte number.
    CWX_UINT32                    m_uiRecvDataLen; ///<recieved data's byte number.
    CwxMsgBlock*                  m_recvMsgData; ///<the recieved msg data
    string                       m_strPeerHost; ///<�Զ�host
    CWX_UINT16                   m_unPeerPort; ///<�Զ�port
    CwxMqTss*                   m_tss;        ///<�����Ӧ��tss����
private:
    static map<CWX_UINT64, CwxMqBinAsyncHandlerSession* > m_sessionMap;  ///<session��map��keyΪsession id
    static list<CwxMqBinAsyncHandlerSession*>            m_freeSession; ///<��Ҫ�رյ�session

};


#endif 
