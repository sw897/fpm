#-------------------------------------------------
#
# Project created by sw 2015-11-05
#
#-------------------------------------------------

QT       -= core gui
TARGET    = fpm
#CONFIG   += console
CONFIG   -= app_bundle
TEMPLATE  = app
DESTDIR   = $$PWD/bin

win32 {
    SOURCES +=\
        src/fpm-win32.c \
        src/win32/getopt.c
    HEADERS += \
        src/win32/getopt.h
    DEFINES += _WINDOWS
    DEFINES -= UNICODE
    LIBS    += -lWS2_32
}

unix {
    SOURCES += \
        src/fpm.c

    target.path = /usr/lib
    INSTALLS += target
    INCLUDEPATH += /usr/include
    DEPENDPATH  += /usr/include
}
