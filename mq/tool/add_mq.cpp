#include "CwxSocket.h"
#include "CwxINetAddr.h"
#include "CwxSockStream.h"
#include "CwxSockConnector.h"
#include "CwxGetOpt.h"
#include "CwxMqPoco.h"
using namespace cwinux;
string g_strHost;
CWX_UINT16 g_unPort = 0;
string g_queue;
string g_user;
string g_passwd;
string g_auth_user;
string g_auth_passwd;
string g_subscribe;
CWX_UINT64 g_sid;
bool   g_commit=false;
CWX_UINT32 g_def_timeout = 0;
CWX_UINT32 g_max_timeout = 0;
///-1��ʧ�ܣ�0��help��1���ɹ�
int parseArg(int argc, char**argv)
{
	CwxGetOpt cmd_option(argc, argv, "H:P:u:p:q:s:d:m:U:W:S:hc");
    int option;

    while( (option = cmd_option.next()) != -1)
    {
        switch (option)
        {
        case 'h':
            printf("create a new queue.\n");
            printf("%s  -H host -P port\n", argv[0]);
            printf("-H: mq server host\n");
            printf("-P: mq server monitor port\n");
            printf("-u: queue's user, it can be empty.\n");
            printf("-p: queue's user passwd, it can be empty.\n");
            printf("-q: queue's name, it can't be empty.\n");
            printf("-c: commit type queue. if no this option, the created queue is non-commit type.\n");
            printf("-s: queue's subscribe. it can be empty for subscribe all message.\n");
            printf("-d: default timeout second for commit queue. it can be zero for using server's default timeout.\n");
            printf("-m: max timeout second for commit queue. it can be zero for using server's max timeout.\n");
            printf("-U: authentication user for queue.\n");
            printf("-W: authentication user password for queue.\n");
            printf("-S: queue's start sid, zero for the current max sid.\n");
            printf("-h: help\n");
            return 0;
        case 'H':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-H requires an argument.\n");
                return -1;
            }
            g_strHost = cmd_option.opt_arg();
            break;
        case 'P':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-P requires an argument.\n");
                return -1;
            }
            g_unPort = strtoul(cmd_option.opt_arg(), NULL, 10);
            break;
        case 'u':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-u requires an argument.\n");
                return -1;
            }
            g_user = cmd_option.opt_arg();
            break;
        case 'p':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-p requires an argument.\n");
                return -1;
            }
            g_passwd = cmd_option.opt_arg();
            break;
        case 'q':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-q requires an argument.\n");
                return -1;
            }
            g_queue = cmd_option.opt_arg();
            break;
        case 'c':
            g_commit = true;
            break;
        case 's':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-s requires an argument.\n");
                return -1;
            }
            g_subscribe = cmd_option.opt_arg();
            break;
        case 'd':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-d requires an argument.\n");
                return -1;
            }
            g_def_timeout = strtoul(cmd_option.opt_arg(),NULL,10);
            break;
        case 'm':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-m requires an argument.\n");
                return -1;
            }
            g_max_timeout = strtoul(cmd_option.opt_arg(),NULL,10);
            break;
        case 'U':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-U requires an argument.\n");
                return -1;
            }
            g_auth_user = cmd_option.opt_arg();
            break;
        case 'W':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-W requires an argument.\n");
                return -1;
            }
            g_auth_passwd = cmd_option.opt_arg();
            break;
        case 'S':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-S requires an argument.\n");
                return -1;
            }
            g_sid = strtoull(cmd_option.opt_arg(),NULL,10);
            break;
        case ':':
            printf("%c requires an argument.\n", cmd_option.opt_opt ());
            return -1;
        case '?':
            break;
        default:
            printf("Invalid arg %s.\n", argv[cmd_option.opt_ind()-1]);
            return -1;
        }
    }
    if (-1 == option)
    {
        if (cmd_option.opt_ind()  < argc)
        {
            printf("Invalid arg %s.\n", argv[cmd_option.opt_ind()]);
            return -1;
        }
    }
    if (!g_strHost.length())
    {
        printf("No host, set by -H\n");
        return -1;
    }
    if (!g_unPort)
    {
        printf("No port, set by -P\n");
        return -1;
    }
    if (!g_queue.length())
    {
        printf("No queue, set by -q\n");
        return -1;
    }
    if (!g_subscribe.length()) g_subscribe = "*";
    return 1;
}

int main(int argc ,char** argv)
{
    int iRet = parseArg(argc, argv);

    if (0 == iRet) return 0;
    if (-1 == iRet) return 1;

    CwxSockStream  stream;
    CwxINetAddr  addr(g_unPort, g_strHost.c_str());
    CwxSockConnector conn;
    if (0 != conn.connect(stream, addr))
    {
        printf("failure to connect ip:port: %s:%u, errno=%d\n", g_strHost.c_str(), g_unPort, errno);
        return 1;
    }
    CwxPackageWriter writer;
    CwxPackageReader reader;
    CwxMsgHead head;
    CwxMsgBlock* block=NULL;
    char szErr2K[2048];
    char const* pErrMsg=NULL;

    CwxMqPoco::init();
    do 
    {
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packCreateQueue(
            &writer,
            block,
            g_queue.c_str(),
            g_user.c_str(),
            g_passwd.c_str(),
            g_subscribe.c_str(),
            g_auth_user.c_str(),
            g_auth_passwd.c_str(),
            g_sid,
            g_commit,
            g_def_timeout,
            g_max_timeout,
            szErr2K
            ))
        {
            printf("failure to pack create-queue package, err=%s\n", szErr2K);
            iRet = 1;
            break;
        }
        if (block->length() != (CWX_UINT32)CwxSocket::write_n(stream.getHandle(),
            block->rd_ptr(),
            block->length()))
        {
            printf("failure to send message, errno=%d\n", errno);
            iRet = 1;
            break;
        }
        CwxMsgBlockAlloc::free(block);
        block = NULL;
        //recv msg
        if (0 >= CwxSocket::read(stream.getHandle(), head, block))
        {
            printf("failure to read the reply, errno=%d\n", errno);
            iRet = 1;
            break;
        }
        if (CwxMqPoco::MSG_TYPE_CREATE_QUEUE_REPLY != head.getMsgType())
        {
            printf("recv a unknow msg type, msg_type=%u\n", head.getMsgType());
            iRet = 1;
            break;

        }
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::parseCreateQueueReply(&reader,
            block,
            iRet,
            pErrMsg,
            szErr2K))
        {
            printf("failure to unpack reply msg, err=%s\n", szErr2K);
            iRet = 1;
            break;
        }
        if (CWX_MQ_ERR_SUCCESS != iRet)
        {
            printf("failure to create queue[%s], err_code=%d, err=%s\n", g_queue.c_str(), iRet, pErrMsg);
            iRet = 1;
            break;
        }
        iRet = 0;
        printf("success to create queue[%s],user=%s,passwd=%s,subscribe=%s,sid=%s,def_timeout=%us,max_timeout=%us\n",
            g_queue.c_str(),
            g_user.c_str(),
            g_passwd.c_str(),
            g_subscribe.c_str(),
            CwxCommon::toString(g_sid, szErr2K, 10),
            g_def_timeout,
            g_max_timeout);
    } while(0);
    if (block) CwxMsgBlockAlloc::free(block);
    CwxMqPoco::destory();
    stream.close();
    return iRet;
}
