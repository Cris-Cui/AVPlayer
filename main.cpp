#include "View/videodialog.h"
#include <QApplication>

#undef main

/**
* @brief 主函数
* @details 程序唯一入口
*
* @param argc 命令参数个数
* @param argv 命令参数指针数组
* @return 程序执行成功与否
*     @retval 0 程序执行成功
*     @retval 1 程序执行失败
*/
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VideoDialog w;
    w.show();
    return a.exec();
}
