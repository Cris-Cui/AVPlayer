QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_DISABLE_DEPRECATED_BEF

SOURCES += \
    main.cpp

include($$PWD/Model/Model.pri)
include($$PWD/View/View.pri)
include($$PWD/Controller/Controller.pri)
include($$PWD/Util/Util.pri)

INCLUDEPATH += \
    $$PWD/ThirdParty/ffmpeg-3.4/include\
    $$PWD/ThirdParty/SDL2-2.0.10/include\
    $$PWD/View/

LIBS += \
    $$PWD/ThirdParty/ffmpeg-3.4/lib/avcodec.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/avdevice.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/avfilter.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/avformat.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/avutil.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/postproc.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/swresample.lib\
    $$PWD/ThirdParty/ffmpeg-3.4/lib/swscale.lib\
    $$PWD/ThirdParty/SDL2-2.0.10/lib/x86/SDL2.lib

RESOURCES += \
    res.qrc


