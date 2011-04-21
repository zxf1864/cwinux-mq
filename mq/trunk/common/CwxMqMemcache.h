#ifndef __MQ_MEMCACHE_H__
#define __MQ_MEMCACHE_H__

#include "MqDispMacro.h"
#include "CwxSockStream.h"

class MqMemcache
{
public:
    enum
    {
        MEMCACHE_MAX_LINE=1023, ///<memcache �����״̬�е���󳤶�
        MEMCACHE_MAX_DATA = 1024 * 1024, ///<memcache��key��data��󳤶�
        MEMCACHE_LINE_BUF = 4096 ///<��ȡline��ʱ��read��buf����
    };
    enum
    {
        MEMCACHE_ERR_SUCCESS = 0,
        MEMCACHE_ERR_CLOSED = 1,
        MEMCACHE_ERR_NOT_STORE = 2, ///add���ڻ�replace������
        MEMCACHE_ERR_EXISTS = 3, ///CAS��key�Ѿ����
        MEMCACHE_ERR_NOT_FOUND = 4, ///CAS��key�����ڻ�delete��key������
        MEMCACHE_ERR_ERROR = 10, ///memcache��"ERROR\r\n"
        MEMCACHE_ERR_CERROR = 11, ///memcache��"CLIENT_ERROR\r\n"
        MEMCACHE_ERR_SERROR = 12, ///memcache��"SERVER_ERROR\r\n"
        MEMCACHE_ERR_NOCONN = 20, ///δ����
        MEMCACHE_ERR_UNKNOWN = 99 ///��������
    };
public :
    ///���캯��
    MqMemcache(char const* szIp, CWX_UINT16 unPort);
    ///��������
    ~MqMemcache();
public:
    //0���ɹ���
    //-1��ʧ��
    int  connect(CWX_UINT32 uiMiliTimeout=0);
    //�ر�����
    void close();
public:
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_CLOSED: ��дʧ�ܶ��ر�����
    int stats(CWX_UINT32 uiMiliTimeout);
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_CLOSED����дʧ�ܶ��ر�����
    ///MEMCACHE_ERR_NOT_STORE
    int add(char const* szKey,
        CWX_UINT32 uiFlag,
        CWX_UINT32 uiExpire,
        char const* szData,
        CWX_UINT32 uiDataLen,
        CWX_UINT32 uiMiliTimeout = 0);
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_CLOSED����дʧ�ܶ��ر�����
    int set(char const* szKey,
        CWX_UINT32 uiFlag,
        CWX_UINT32 uiExpire,
        char const* szData,
        CWX_UINT32 uiDataLen,
        CWX_UINT32 uiMiliTimeout = 0);
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_CLOSED����дʧ�ܶ��ر�����
    ///MEMCACHE_ERR_EXISTS��key��cas�ı���޷��޸�
    ///MEMCACHE_ERR_NOT_FOUND��key������
    int cas(char const* szKey,
        CWX_UINT32 uiFlag,
        CWX_UINT32 uiExpire,
        char const* cas,
        char const* szData,
        CWX_UINT32 uiDataLen,
        CWX_UINT32 uiMiliTimeout = 0);
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_NOT_FOUND��û�з���
    ///MEMCACHE_ERR_CLOSED����ȡʧ�ܣ����ӹر�
    int get(char const* szKey, CWX_UINT32 uiMiliTimeout = 0);
    ///MEMCACHE_ERR_SUCCESS���ɹ�
    ///MEMCACHE_ERR_NOT_FOUND��û�з���
    ///MEMCACHE_ERR_CLOSED����ȡʧ�ܣ����ӹر�
    int gets(char const* szKey, CWX_UINT32 uiMiliTimeout = 0);
public:
    ///�Ƿ�����
    bool isConn() const
    {
        return m_stream.getHandle() != CWX_INVALID_HANDLE;
    }
    ///��ȡ���Ӿ��
    CWX_HANDLE getHandle() const
    {
        return m_stream.getHandle();
    }
    ///��ȡget��gets���ص�flags
    CWX_UINT32 getFlags() const
    {
        return m_uiFlags;
    }
    ///��ȡgets���ص�cas
    char const* getCas() const
    {
        return m_szCas;
    }
    ///��ȡget��gets��stats���ص�value���ȡ�
    CWX_UINT32 getDataLen() const
    {
        return m_uiDataLen;
    }
    ///��ȡget��gets��stats���ص�value
    char const* getData() const
    {
        return m_szData;
    }
    ///��ȡ����Ĵ�����Ϣ
    char const* getErrMsg() const
    {
        return m_szErr2K;
    }
private:
    ///��ȡ����Ķ�ȡ���ݴ�С
    CWX_UINT32 getCachedReadBufSize() const
    {
        return m_uiReadBufEnd>m_uiReadBufStart?m_uiReadBufEnd - m_uiReadBufStart:0;
    }
    ///��ȡ����Ķ�ȡ����
    char const* getCachedReadBuf() const
    {
        return m_szReadBuf + m_uiReadBufStart;
    }
    //��ȡһ�У����ܳ���MEMCACHE_MAX_LINE����������\r\n��
    //����ֵ -1��ʧ�ܣ����򷵻��е��ֽ�����
    int readLine(CwxTimeouter* timer);
    //��ȡ����
    //����ֵ -1��ʧ�ܣ����򷵻��ֽ�����
    int readData(CwxTimeouter* timer);
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"END\r\n"
    bool isEndLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "END\r\n", uiLen)==0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"STORED\r\n"
    bool isStoredLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "STORED\r\n", uiLen)==0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"NOT_STORED\r\n"
    bool isNotStoredLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "NOT_STORED\r\n", uiLen)==0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"EXISTS\r\n"
    bool isExistLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "EXISTS\r\n", uiLen) == 0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"NOT_FOUND\r\n"
    bool isNotFoundLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "NOT_FOUND\r\n", uiLen) == 0;
    }
    //�ж���ǰ��ȡ��memcache�����У��Ƿ�Ϊ"ERROR\r\n"
    bool isErrorLine(CWX_UINT32 uiLen)
    {
        return memcmp(m_szLine, "ERROR\r\n", uiLen) == 0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"CLIENT_ERROR"
    bool isClientErrorLine()
    {
        return memcmp(m_szLine, "CLIENT_ERROR", strlen("CLIENT_ERROR")) == 0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"SERVER_ERROR"
    bool isServerErrorLine()
    {
        return memcmp(m_szLine, "SERVER_ERROR", strlen("SERVER_ERROR"))==0;
    }
    //�жϵ�ǰ��ȡ��memcache�����У��Ƿ�Ϊ"VALUE" ��
    bool isValueLine()
    {
        return memcmp(m_szLine, "VALUE ", strlen("VALUE ")) ==0;
    }
private:
    CwxSockStream   m_stream; ///<memcache������stream
    CWX_UINT16      m_unPort; ///<memcache�Ķ˿ں�
    string          m_Ip;     ///<memcache��ip��ַ
    CWX_UINT32      m_uiFlags; ///<��ȡkey��flags
    char            m_szCas[64]; ///<��ȡkey��casֵ
    CWX_UINT32      m_uiDataLen; ///<��ȡkey��data��stat���صĳ���
    char            m_szData[MEMCACHE_MAX_DATA]; ///<key��data��stat���ص�����
    char            m_szLine[MEMCACHE_MAX_LINE + 1]; ///<��ȡ���е�����
    char            m_szReadBuf[MEMCACHE_LINE_BUF]; ///<read lineʱ��buf��
    CWX_UINT32      m_uiReadBufStart;               ///<m_szReadBuf��Ч���ݵĿ�ʼλ��
    CWX_UINT32      m_uiReadBufEnd;                 ///<m_szReadBuf��Ч���ݵĽ���λ��
    char            m_szErr2K[2048]; ///<������Ϣ
};

#endif

