#include "View/videodialog.h"
#include <QApplication>

#undef main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VideoDialog w;
    w.show();
    return a.exec();
}
