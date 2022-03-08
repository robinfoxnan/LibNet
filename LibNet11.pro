QT += core
QT -= gui

CONFIG += c++11

TARGET = LibNet11
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += \
    src/Buffer.cc \
    src/Channel.cc \
    src/EventLoopThreadPool.cc \
    src/InetAddress.cc \
    src/IOEventLoop.cc \
    src/Socket.cc \
    src/TcpConnection.cc \ 
    src/PollerFactory.cc \
    src/IPooler.cc \
    src/SelectPoller.cc \
    src/linux/SocketsApi.cc \
    src/linux/EPollPoller.cc \
    src/linux/PollPoller.cc \
    http/http_parser.c \
    http/Detail.cc \
    http/HttpClient.cc \
    src/log4z.cpp \
    src/LogWriter.cpp \
    src/TimerHeap.cpp \
    src/SocketSSL.cpp \
    test/testClientSyn.cpp


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    include/Buffer.h \
    include/Callbacks.h \
    include/Channel.h \
    include/CommonHeader.h \
    include/CommonLog.h \
    include/CounterLatch.h \
    include/linux/Endian.h \
    include/EventLoopThreadPool.h \
    include/InetAddress.h \
    include/IOEventLoop.h \
    include/IPoller.h \
    include/noncopyable.h \
    include/Socket.h \
    include/SocketsApi.h \
    include/StringPiece.h \
    include/TcpConnection.h \
    include/pollerfactory.h \
    include/MyTimer.h \
    include/SelectPoller.h \
    include/linux/Endian.h \
    include/linux/EPollPoller.h \
    include/linux/PollPoller.h \
    http/http_parser.h \
    http/Detail.h \
    http/HttpClient.h \
    include/log4wraper.h \
    include/log4z.h \
    include/log4zwraper.h \
    include/LogWriter.h \
    include/Util.h \
    include/TimerHeap.h

LIBS += -L"/usr/local/lib" -lcrypto -lssl

QMAKE_CXXFLAGS +=  -Wno-unused-parameter



