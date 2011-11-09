
#ifndef __ZK_JPOOL_ADAPTER_H__
#define __ZK_JPOOL_ADAPTER_H__

#include "ZkAdaptor.h"

class ZkJPoolAdapter:public ZkAdapter
{
public:
	///���캯��
	ZkJPoolAdapter(string const& strHost,
		CWX_UINT32 uiRecvTimeout=ZK_DEF_RECV_TIMEOUT_MILISECOND);
	///��������
	virtual ~ZkJPoolAdapter(); 
	///���ӽ���
	virtual void onConnect(){
		printf("Success to connect %s\n", m_strHost.c_str());
	}
	///��Ȩʧ��
	virtual void onFailAuth(){
		printf("Failure auth to %s\n", m_strHost.c_str());
	}
	///SessionʧЧ
	virtual void onExpired(){
		printf("Session expired for %s\n", m_strHost.c_str());
	}
	///������Ϣ
	virtual void onOtherEvent(int type, int state, const char *path){
		printf("Recv event for %s, type=%d  state=%d  path=%s\n", m_strHost.c_str(), type, state, path);
	}
};

#endif /* __ZK_ADAPTER_H__ */
