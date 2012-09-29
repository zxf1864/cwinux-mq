﻿#include "CwxSocket.h"
#include "CwxINetAddr.h"
#include "CwxSockStream.h"
#include "CwxSockConnector.h"
#include "CwxGetOpt.h"
#include "CwxMqPoco.h"
#include "CwxFile.h"

using namespace cwinux;
string g_strHost;
CWX_UINT16 g_unPort = 0;
string g_user;
string g_passwd;
string     g_data;
string     g_file;
char*       g_szData = NULL;
CWX_UINT32 g_uiDataLen = 0;
string     g_sign;
bool       g_zip=false;
///-1：失败；0：help；1：成功
int parseArg(int argc, char**argv)
{
    CwxGetOpt cmd_option(argc, argv, "H:P:u:p:d:f:m:zh");
    int option;
    while( (option = cmd_option.next()) != -1)
    {
        switch (option)
        {
        case 'h':
            printf("Send a message to the mq server.\n");
            printf("%s  -H host -P port\n", argv[0]);
            printf("-H: mq server recieve host\n");
            printf("-P: mq server recieve port\n");
            printf("-u: mq server's recieve user.\n");
            printf("-p: mq server's recieve user password.\n");
            printf("-d: message's data.\n");
            printf("-m: signature type, %s or %s. no signature by default\n", CWX_MQ_MD5, CWX_MQ_CRC32);
            printf("-f: file name which contains message's data.\n");
            printf("-z: compress sign. no compress by default.\n");
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
        case 'd':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-d requires an argument.\n");
                return -1;
            }
            g_data = cmd_option.opt_arg();
            break;
        case 'm':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-m requires an argument.\n");
                return -1;
            }
            g_sign = cmd_option.opt_arg();
            if ((g_sign != CWX_MQ_CRC32) && (g_sign != CWX_MQ_MD5))
            {
                printf("signature must be %s or %s\n", CWX_MQ_MD5, CWX_MQ_CRC32);
            }
            break;
        case 'f':
            if (!cmd_option.opt_arg() || (*cmd_option.opt_arg() == '-'))
            {
                printf("-f requires an argument.\n");
                return -1;
            }
            g_file = cmd_option.opt_arg();
            break;
        case 'z':
            g_zip = true;
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
    if (!g_data.length() && !g_file.length())
    {
        printf("No data, set by -d or -f\n");
        return -1;
    }
    if (g_file.length())
    {
        if (!CwxFile::isFile(g_file.c_str()))
        {
            printf("File[%s] doesn't exist or isn't a valid file.\n", g_file.c_str());
            return -1;
        }
        off_t size = CwxFile::getFileSize(g_file.c_str());
        if (-1 == size)
        {
            printf("Failure to get file size, file=%s, errno=%d\n", g_file.c_str(), errno);
            return -1;
        }
        if (size > CWX_MQ_MAX_MSG_SIZE)
        {
            printf("Data in file is too long, max is %u.\n", CWX_MQ_MAX_MSG_SIZE);
            return -1;
        }
        g_szData = (char*)malloc(size);
        g_uiDataLen = size;
        FILE* fd = fopen(g_file.c_str(),"r");
        if (!fd)
        {
            printf("Failure to open file:%s, errno=%d\n", g_file.c_str(), errno);
            return -1;
        }
        if (size != (CWX_UINT32)fread(g_szData, 1, size, fd))
        {
            printf("Failure to read file:%s, errno=%d\n", g_file.c_str(), errno);
            fclose(fd);
            return -1;
        }
        fclose(fd);
    }
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
    CwxKeyValueItem item;

    if (g_file.length()){
        item.m_szData = g_szData;
        item.m_uiDataLen = g_uiDataLen;
    }else{
        item.m_szData = (char*)g_data.c_str();
        item.m_uiDataLen = g_data.length();
    }
    do {
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::packRecvData(
            &writer,
            block,
            0,
            item,
            g_group,
            g_user.c_str(),
            g_passwd.c_str(),
            g_sign.c_str(),
            g_zip,
            szErr2K
            ))
        {
            printf("failure to pack message package, err=%s\n", szErr2K);
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
        if (CwxMqPoco::MSG_TYPE_RECV_DATA_REPLY != head.getMsgType())
        {
            printf("recv a unknow msg type, msg_type=%u\n", head.getMsgType());
            iRet = 1;
            break;
        }
        CWX_UINT64 ullSid;
        if (CWX_MQ_ERR_SUCCESS != CwxMqPoco::parseRecvDataReply(&reader,
            block,
            iRet,
            ullSid,
            pErrMsg,
            szErr2K))
        {
            printf("failure to unpack reply msg, err=%s\n", szErr2K);
            iRet = 1;
            break;
        }
        if (CWX_MQ_ERR_SUCCESS != iRet)
        {
            printf("failure to send message, err_code=%d, err=%s\n", iRet, pErrMsg);
            iRet = 1;
            break;
        }
        iRet = 0;
        printf("success to send msg, data's sid=%s\n",
            CwxCommon::toString(ullSid, szErr2K, 10));
    } while(0);
    if (g_szData) free(g_szData);
    if (block) CwxMsgBlockAlloc::free(block);
    stream.close();
    return iRet;
}
