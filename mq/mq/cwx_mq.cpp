#include "CwxAppProcessMgr.h"
#include "CwxMqApp.h"

int main(int argc, char** argv)
{
    //����mq��app����ʵ��
    CwxMqApp* pApp = new CwxMqApp();
    //��ʼ��˫���̹�����
    if (0 != CwxAppProcessMgr::init(pApp)) return 1;
    //����˫���̣�һ��Ϊ���mq���̵ļ�ؽ��̣�һ��Ϊ�ṩmq����Ĺ������̡�
    CwxAppProcessMgr::start(argc, argv, 200, 300);
    return 0;
}
