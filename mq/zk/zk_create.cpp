#include "ZkJPoolAdaptor.h"
#include "CwxGetOpt.h"
#include "CwxTimeValue.h"
using namespace cwinux;

string g_strHost;
string g_strNode;
string g_strValue;
list<string> g_auth;
list<string>  g_priv;
///-1��ʧ�ܣ�0��help��1���ɹ�
int parseArg(int argc, char**argv)
{
	CwxGetOpt cmd_option(argc, argv, "H:n:d:a:h");
    int option;
    while( (option = cmd_option.next()) != -1)
    {
        switch (option)
        {
        case 'h':
            printf("create zookeeper node.\n");
			printf("%s  -H host:port -n node -d data -a usr:passwd -l privilege\n", argv[0]);
			printf("-H: zookeeper's host:port\n");
            printf("-n: node name to create, it's full path.\n");
			printf("-d: value for node.\n");
			printf("-a: auth user's user:passwd. it can be multi.\n");
			printf("-l: node's acl. it can be multi. it's value can be:\n");
			printf("    all               :  any privilege for any user;\n");
            printf("    self              : any privilege for creator; \n");
			printf("    read              : read for any user;\n");
			printf("    user:passwd:acrwd : digest auth for [user] with [passwd], \n");
			printf("          admin(a), create(c), read(r), write(w), delete(d)\n");
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
        case 'n':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-n requires an argument.\n");
                return -1;
            }
            g_strNode = cmd_option.opt_arg();
            break;
		case 'd':
			if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
			{
				printf("-d requires an argument.\n");
				return -1;
			}
			g_strValue = cmd_option.opt_arg();
			break;
		case 'a':
			if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
			{
				printf("-a requires an argument.\n");
				return -1;
			}
			g_auth.push_back(cmd_option.opt_arg());
			break;
		case 'l':
			if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
			{
				printf("-l requires an argument.\n");
				return -1;
			}
			g_priv.push_back(cmd_option.opt_arg());
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
	if (!g_strNode.length())
	{
		printf("No node, set by -n\n");
		return -1;
	}
    return 1;
}

int main(int argc ,char** argv)
{
    int iRet = parseArg(argc, argv);

    if (0 == iRet) return 0;
    if (-1 == iRet) return 1;

	ZkJPoolAdaptor zk(g_strHost);

	if (0 != zk.init()){
		printf("Failure to init zk, err=%s\n", zk.getErrMsg());
		return -1;
	}
	if (0 != zk.connect())
	{
		printf("Failure to connect zk, err=%s\n", zk.getErrMsg());
		return -1;
	}
	
	int timeout = 5000;
	while(timeout > 0){
		if (!zk.isConnected()){
			timeout --;
			ZkAdaptor::sleep(1);
			continue;
		}
		if (!zk.createNode(g_strNode, g_strValue.c_str(), g_strValue.length(), 0x1f, g_strAuth.length()?"digest":"", g_strAuth))
		{
			printf("Failure to create node, err=%s\n", zk.getErrMsg());
			return 1;
		}
		printf("Success to create node for %s\n", g_strNode.c_str());
		return 0;
	}
	printf("Timeout for connect zk:%s\n", g_strHost.c_str());
    return 1;
}
