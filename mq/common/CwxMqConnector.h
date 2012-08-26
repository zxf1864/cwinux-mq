#ifndef __CWX_MQ_CONNECTOR_H__
#define __CWX_MQ_CONNECTOR_H__

#include "CwxPre.h"
#include "CwxType.h"
#include "CwxErrGuard.h"
#include "CwxNetMacro.h"
#include "CwxINetAddr.h"
#include "CwxSockStream.h"
#include "CwxSockBase.h"

CWINUX_USING_NAMESPACE

///��timeoutָ����ʱ���ڣ�����addrָ���unConnNum�����ӣ�ֻҪһ��ʧ����ʧ��
class CwxMqConnector
{
public:
    ///�������ӡ�����ֵ��0���ɹ���-1��ʧ�ܡ�
    static int connect (CwxINetAddr const& addr, ///<���ӵĵ�ַ
        CWX_UINT16 unConnNum, ///<���ӵ�����
        int* fd, ///<����fd����ֵ����ռ����ⲿ��֤��
        CwxTimeouter* timeout=NULL, ///<���ӳ�ʱʱ��
        bool reuse_addr = false, ///<�Ƿ����ö˿�
        CWX_NET_SOCKET_ATTR_FUNC fn=NULL, ///<socket���õ�function
        void* fnArg=NULL ///<socket����function�Ĳ���
    );
private:
    ///�ȴ�������ɲ�����
    static int complete (set<int>& fds, ///<��������Ӽ���
        CwxTimeouter* timeout ///<��ʱʱ��
        );
private:
    ///Ĭ�Ϲ��캯��
    CwxMqConnector(){}
    ///��������.
    ~CwxMqConnector(void){}
};

#endif
