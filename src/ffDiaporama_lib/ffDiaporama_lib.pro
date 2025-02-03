# ======================================================================
#  This file is part of ffDiaporama
#  ffDiaporama is a tool to make diaporama as video
#  Copyright (C) 2011-2014 Dominique Levray <domledom@laposte.net>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
# ======================================================================

isEmpty(PREFIX) {
    PREFIX = /usr
}

CONFIG   += qt thread
QT       += core gui svg sql xml
unix {
    QMAKE_CXX += -msse4.1
}

greaterThan(QT_MAJOR_VERSION,4) {
    # QT5 version
    QT += widgets concurrent help
} else {
    # QT4 version
    CONFIG += help
}

QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual

QMAKE_STRIP = echo
APPFOLDER   = ffDiaporama
TARGET      = ffDiaporama_lib
TEMPLATE    = lib
CONFIG     += staticlib

OTHER_FILES += readme.txt

SOURCES  += BasicDefines.cpp \
            cBackgroundComboBox.cpp \
            cBaseAppConfig.cpp \
            cBaseBrushDefinition.cpp \
            cBrushComboBox.cpp \
            cColorComboBox.cpp \
            cCustomIcon.cpp \
            cDatabase.cpp \
            cGrdOrientationComboBox.cpp \
            cLuLoImageCache.cpp \
            cSaveWindowPosition.cpp \
            cSpeedWave.cpp \
            cSpeedWaveComboBox.cpp \
            cTexteFrameComboBox.cpp \
            cTextFrame.cpp \
            ImageFilters.cpp \
            QCustomComboBox.cpp \
            QCustomHorizSplitter.cpp \
            Shape.cpp \
            ocv.cpp \
            SSE_Check.cpp

HEADERS  += BasicDefines.h \
            cBackgroundComboBox.h \
            cBaseAppConfig.h \
            cBaseBrushDefinition.h \
            cBrushComboBox.h \
            cColorComboBox.h \
            cCustomIcon.h \
            cDatabase.h \
            cGrdOrientationComboBox.h \
            cLuLoImageCache.h \
            cSaveWindowPosition.h \
            cSpeedWave.h \
            cSpeedWaveComboBox.h \
            cTexteFrameComboBox.h \
            cTextFrame.h \
            ImageFilters.h \
            QCustomComboBox.h \
            QCustomHorizSplitter.h \
            Shape.h \
            ocv.h \
            SSE_Check.h


win32 {
  #QMAKE_LFLAGS_CONSOLE += /NODEFAULTLIB:LIBCMT
  #QMAKE_LFLAGS_WINDOWS += /NODEFAULTLIB:LIBCMT
   contains(QMAKE_HOST.arch,x86_64) {
   	CONFIG(debug, debug|release)  {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama/ffdiaporama_lib/debug
   	}
	   CONFIG(release, debug|release) {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama/ffdiaporama_lib/release
      }
   }
   else {
   	CONFIG(debug, debug|release)  {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama_x86/ffdiaporama_lib/debug
   	}
	   CONFIG(release, debug|release) {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama_x86/ffdiaporama_lib/release
      }
   }
   CONFIG(debug, debug|release)  {
	   OBJECTS_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}
	   UI_DIR  = $${OBJECTS_DIR_BASE}\\$${TARGET}\\ui
	   MOC_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}\\moc
	   RCC_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}\\moc
	   OBJMOC  = $${OBJECTS_DIR_BASE}\\$${TARGET}\\mocobj
   }
   CONFIG(release, debug|release) {
	   OBJECTS_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}
	   UI_DIR  = $${OBJECTS_DIR_BASE}\\$${TARGET}\\ui
	   MOC_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}\\moc
	   RCC_DIR = $${OBJECTS_DIR_BASE}\\$${TARGET}\\moc
	   OBJMOC  = $${OBJECTS_DIR_BASE}\\$${TARGET}\\mocobj
   }
   PRECOMPILED_HEADER = ffdiaporamalib_inc.h
}

#--------------------------------------------------------------
# INSTALLATION
#--------------------------------------------------------------
unix:!symbian {
    TARGET.path   = $$PREFIX/lib
    #TARGET.files = $$TARGET
    INSTALLS += TARGET
}
!win32 {
   QMAKE_CXXFLAGS += -Wno-zero-as-null-pointer-constant
}

include(../../ffdiaporama.pri)
contains(DEFINES,qmake_destdir){
DESTDIR=
}
message("in lib.pro DESTDIR is $$DESTDIR")
