#include "CwxAppProcessMgr.h"
#include "CwxMqApp.h"

int main(int argc, char** argv)
{
    //����dispatch��app����ʵ��
    CwxMqApp* pApp = new CwxMqApp();
    //��ʼ��˫���̹�����
    if (0 != CwxAppProcessMgr::init(pApp)) return 1;
    //����˫���̣�һ��Ϊ���Dispatch���̵ļ�ؽ��̣�һ��Ϊ�ṩDispatch����Ĺ������̡�
    CwxAppProcessMgr::start(argc, argv, 200, 300);
    return 0;
}
