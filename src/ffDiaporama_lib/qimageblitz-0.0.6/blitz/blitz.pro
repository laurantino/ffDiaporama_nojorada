######################################################################
# Automatically generated by qmake (3.0) So Okt 11 00:23:05 2015
######################################################################

TEMPLATE = lib
TARGET = blitz
INCLUDEPATH += .

DEFINES += MAKE_QIMAGEBLITZ_LIB
include(../blitz.pri)
# Input
HEADERS += blitzcpu.h \
           qimageblitz.h \
           qimageblitz_export.h \
           private/blitz_p.h \
           private/inlinehsv.h \
           private/interpolate.h \
           ocv.h
SOURCES += blitz.cpp \
           blitzcpu.cpp \
           colors.cpp \
           convolve.cpp \
           gradient.cpp \
           histogram.cpp \
           scale.cpp \
           scalefilter.cpp \
           ocv.cpp
