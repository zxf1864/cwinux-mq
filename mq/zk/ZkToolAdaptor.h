#ifndef __ZK_TOOL_ADAPTOR_H__
#define __ZK_TOOL_ADAPTOR_H__

#include "ZkAdaptor.h"

class ZkToolAdaptor:public ZkAdaptor
{
public:
	///���캯��
	ZkToolAdaptor(string const& strHost,
		uint32_t uiRecvTimeout=ZK_DEF_RECV_TIMEOUT_MILISECOND);
	///��������
	virtual ~ZkToolAdaptor(); 
	///���ӽ����Ļص��������еײ��zk�̵߳���
	virtual void onConnect()
	{
		printf("Success to connect %s\n", getHost().c_str());
	}

	///���ڽ�����ϵ�Ļص��������еײ��zk�̵߳���
	virtual void onAssociating()
	{
		printf("Associating to zk %s\n", getHost().c_str());
	}
	///���ڽ������ӵĻص��������еײ��zk�̵߳���
	virtual void onConnecting()
	{
		printf("Connecting to zk %s\n", getHost().c_str());
	}

	///��Ȩʧ�ܵĻص��������еײ��zk�̵߳���
	virtual void onFailAuth()
	{
		printf("Failure auth to zk %s\n", getHost().c_str());
	}

	///SessionʧЧ�Ļص��������еײ��zk�̵߳���
	virtual void onExpired()
	{
		printf("Expired to zk %s\n", getHost().c_str());
	}
	/**
	*@brief  watch��node�����¼��Ļص��������еײ��zk�̵߳��á�Ӧ����zoo_exists��watch��
	*@param [in] zk��watcher��state
	*@param [in] path watch��path.
	*@return void.
	*/
	virtual void onNodeCreated(int state, char const* path)
	{
		printf("Node[%s] is created for zk %s, state=%d\n", path, getHost().c_str(), state);
	}

	/**
	*@brief  watch��nodeɾ���¼��Ļص��������еײ��zk�̵߳��á�Ӧ���� zoo_exists��zoo_get��watch��
	*@param [in] zk��watcher��state
	*@param [in] path watch��path.
	*@return void.
	*/
	virtual void onNodeDeleted(int state, char const* path)
	{
		printf("Node[%s] is deleted for zk %s, state=%d\n", path, getHost().c_str(), state);
	}

	/**
	*@brief  watch��node�޸��¼��Ļص��������еײ��zk�̵߳��á�Ӧ���� zoo_exists��zoo_get��watch��
	*@param [in] zk��watcher��state
	*@param [in] path watch��path.
	*@return void.
	*/
	virtual void onNodeChanged(int state, char const* path)
	{
		printf("Node[%s] is changed for zk %s, state=%d\n", path, getHost().c_str(), state);
	}

	/**
	*@brief  watch��node���ӱ���¼��Ļص��������еײ��zk�̵߳��á�Ӧ����zoo_get_children��watch��
	*@param [in] zk��watcher��state
	*@param [in] path watch��path.
	*@return void.
	*/
	virtual void onNodeChildChanged(int state, char const* path)
	{
		printf("Node[%s]'s child is changed for zk %s, state=%d\n", path, getHost().c_str(), state);
	}

	/**
	*@brief  zkȡ��ĳ��wathc��֪ͨ�¼��Ļص��������еײ��zk�̵߳��á�
	*@param [in] zk��watcher��state
	*@param [in] path watch��path.
	*@return void.
	*/
	virtual void onNoWatching(int state, char const* path)
	{
		printf("Node[%s]'s watch is removed for zk %s, state=%d\n", path, getHost().c_str(), state);
	}

	///������Ϣ
	virtual void onOtherEvent(int type, int state, const char *path){
		printf("Recv event for %s, type=%d  state=%d  path=%s\n", getHost().c_str(), type, state, path);
	}
};

#endif /* __ZK_ADAPTER_H__ */
