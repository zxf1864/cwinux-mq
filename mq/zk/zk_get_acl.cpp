#include "zk_common.h"


string g_strHost;
string g_strNode;
string g_strOut;
list<string> g_auth;
///-1��ʧ�ܣ�0��help��1���ɹ�
int parseArg(int argc, char**argv)
{
	ZkGetOpt cmd_option(argc, argv, "H:n:a:o:h");
    int option;
    while( (option = cmd_option.next()) != -1)
    {
        switch (option)
        {
        case 'h':
            printf("get zookeeper node's ACL.\n");
			printf("%s  -H host:port -n node [-o output file] [-a usr:passwd]\n", argv[0]);
			printf("-H: zookeeper's host:port\n");
            printf("-n: node name to create, it's full path.\n");
			printf("-a: auth user's user:passwd. it can be multi.\n");
			printf("-o: output file, default is stdout\n");
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
		case 'a':
			if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
			{
				printf("-a requires an argument.\n");
				return -1;
			}
			g_auth.push_back(cmd_option.opt_arg());
			break;
		case 'o':
			if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
			{
				printf("-o requires an argument.\n");
				return -1;
			}
			g_strOut = cmd_option.opt_arg();
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


//0:success
//1:��������
//2:ִ�н������
int main(int argc ,char** argv)
{
	FILE * outFd = NULL;
    int iRet = parseArg(argc, argv);

    if (0 == iRet) return 0;
    if (-1 == iRet) return 1;

	if (g_strOut.length())
	{
		outFd = fopen(g_strOut.c_str(), "w+b");
		if (!outFd){
			printf("Failure to open output file:%s, errno=%d\n", g_strOut.c_str(), errno);
			return 1;
		}
	}

	ZkToolAdaptor zk(g_strHost);

	if (0 != zk.init()){
		output(outFd, 2, zk.getErrCode(), "msg:  Failure to init zk, err=%s\n", zk.getErrMsg());
		if (outFd) fclose(outFd);
		return 2;
	}

	if (0 != zk.connect())
	{
		output(outFd, 2, zk.getErrCode(), "msg:  Failure to connect zk, err=%s\n", zk.getErrMsg());
		if (outFd) fclose(outFd);
		return 2;
	}
	
	int timeout = 5000;
	while(timeout > 0){
		if (!zk.isConnected()){
			timeout --;
			ZkAdaptor::sleep(1);
			continue;
		}
		//add auth
		if (g_auth.size())
		{
			list<string>::iterator iter = g_auth.begin();
			while(iter != g_auth.end())
			{
				if (!zk.addAuth("digest", iter->c_str(), iter->length(), 3000))
				{
					output(outFd, 2, 0,"msg:  Failure to auth, err=%s\n", zk.getErrMsg());
					if (outFd) fclose(outFd);
					return 2;
				}
				iter++;
			}
		}
		struct ACL_vector acl;
		struct Stat stat;
		int ret = zk.getAcl(g_strNode.c_str(), acl, stat);
		if (-1 == ret){
			output(outFd, 2, zk.getErrCode(), "msg:  Failure to get node's acl, err=%s\n", zk.getErrMsg());
			if (outFd) fclose(outFd);
			return 2;
		}
		if (0 == ret){
			output(outFd, 2, zk.getErrCode(), NULL, "msg:  node doesn't exist\n");
			if (outFd) fclose(outFd);
			return 2;
		}
		output(outFd, 0, 0, "node:  %s\nmsg:  success\n", g_strNode.c_str());
		list<string> acls;
		string strStat;
		ZkAdaptor::dumpStat(stat, strStat);
		ZkAdaptor::dumpAcl(acl, acls);
		if (outFd){
			fprintf(outFd, "stat:\n");
			fwrite(strStat.c_str(), 1, strStat.length(), outFd);
			fprintf(outFd, "acl:\n");
			list<string>::iterator iter = acls.begin();
			while(iter != acls.end())
			{
				fprintf(outFd, "%s\n", iter->c_str());
				iter++;
			}
			fclose(outFd);
		}else{
			printf("stat:\n");
			printf(strStat.c_str());
			printf("acl:\n");
			list<string>::iterator iter = acls.begin();
			while(iter != acls.end())
			{
				printf( "%s\n", iter->c_str());
				iter++;
			}
		}
		free(acl.data);
		return 0;
	}
	output(outFd, 2, 0, NULL, "msg:  Timeout to connect zk\n");
	if (outFd) fclose(outFd);
	return 2;
}
