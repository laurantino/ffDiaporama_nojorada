# ======================================================================
#  This file is part of ffDiaporama
#  ffDiaporama is a tools to make diaporama as video
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

TARGET = ffDiaporama
TEMPLATE = app

# for filters in souretree (visual studio)
#CONFIG -= flat

VERSION = 3.0.0
win32:TARGET_EXT = .exe

isEmpty(PREFIX) {
    PREFIX = /usr
}

CONFIG += qt thread

greaterThan(QT_MAJOR_VERSION,4) {
    # QT5 version
    QT += widgets concurrent help multimedia quick quickwidgets quickcontrols2
} else {
    # QT4 version
    CONFIG += help 
    QT += multimedia
}

QT          += core gui xml network svg sql
lessThan(QT_MAJOR_VERSION,6) {
win32:QT += winextras
}

QMAKE_STRIP  = echo
APPFOLDER    = ffDiaporama

include(../../ffdiaporama.pri)

#--------------------------------------------------------------
# Add link to ffDiaporama_lib
#--------------------------------------------------------------
INCLUDEPATH += ../ffDiaporama_lib
LIBS += -lffDiaporama_lib

#--------------------------------------------------------------
# DEFINES $$DESTDIR DIRECTORIES, COMMON INCLUDES AND COMMON LIBS
#--------------------------------------------------------------

DEFINES +=SHARE_DIR=\\\"$$PREFIX\\\"

unix {
   QMAKE_CXX += -msse4.1

   LIBS   += -L../ffDiaporama_lib

   CFLAGS += -W"Missing debug information for"
   QMAKE_CXXFLAGS_WARN_ON += -Wno-overloaded-virtual -Wno-inconsistent-missing-override

   contains(DEFINES,Q_OS_SOLARIS) {

        HARDWARE_PLATFORM = $$system(uname -m)
        contains(HARDWARE_PLATFORM,x86_64) {
            DEFINES+=Q_OS_SOLARIS64
            message("Solaris x86_64 build")
        } else {
            DEFINES+=Q_OS_SOLARIS32
            message("Solaris x86 build")
        }
        message("Use ffmpeg in /opt/gnu/include")
        INCLUDEPATH += /opt/gnu/include
        DEFINES     += USELIBSWRESAMPLE
        LIBS        += -lswresample

    } else {

        HARDWARE_PLATFORM = $$system(uname -m)
        contains(HARDWARE_PLATFORM,x86_64) {
            DEFINES+=Q_OS_LINUX64
            message("Linux x86_64 build")
        } else {
            DEFINES+=Q_OS_LINUX32
            message("Linux x86 build")
        }

#        exists(/opt/ffmpeg/include/libswresample/swresample.h) {         #------ conditionnaly includes from Sam Rog packages for Ubuntu
#            message("Use SAM ROG Packages from /opt/ffmpeg")
#            INCLUDEPATH += /opt/ffmpeg/include/
#            LIBS        += -L"/opt/ffmpeg/lib"
#            DEFINES += USELIBSWRESAMPLE
#            LIBS    += -lswresample                                             #------ conditionnaly include libswresample
#        } else:exists(/usr/include/ffmpeg/libswresample/swresample.h) {         #------ Specific for Fedora
#            message("Use ffmpeg in /usr/include/ffmpeg")
#            DEFINES += USELIBSWRESAMPLE
#            INCLUDEPATH += /usr/include/ffmpeg/
#            LIBS    += -lswresample                                             #------ conditionnaly include libswresample
#        } else:exists(/usr/include/libswresample/swresample.h) {                #------ Specific for openSUSE
#            message("Use ffmpeg in /usr/include")
#            INCLUDEPATH += /usr/include/
#            DEFINES += USELIBSWRESAMPLE
#            LIBS    += -lswresample                                             #------ conditionnaly include libswresample
#        } else:exists(/usr/include/libavresample/avresample.h) {
#            message("Use libav 9 in /usr/include")
#            DEFINES += USELIBAVRESAMPLE
#            LIBS    += -lavresample                                             #------ conditionnaly include libavresample
#            INCLUDEPATH += /usr/include/
#        } else {
#            message("Use libav 0.8+taglib in /usr/include")
#            LIBS        += -ltag                                                #------ TAGlib is used only with LIBAV 8
#            DEFINES     += USETAGLIB
#            DEFINES     += HAVE_CONFIG_H                                        #------ specific for TAGLib
#            DEFINES     += TAGLIB_STATIC                                        #------ specific for TAGLib
#            INCLUDEPATH += /usr/include/
#        }

    }
    #ffmpeg via pkg-config
    CONFIG += link_pkgconfig
    PKGCONFIG += libavformat libavcodec libavutil libswscale libavfilter libswresample

    #LIBS        += -L"../exiv2"                                             #------ Exiv2
    #LIBS        += -lexiv2_own                                                  #------ Exiv2
    #LIBS += -lpthread -lz -ldl
    LIBS        += -lexiv2                                                  #------ Exiv2

    OTHER_FILES += ffDiaporama.rc \
                   ffdiaporama.ico \    # icon file to be install on windows system
                   ffDiaporama.url      # URL file to be install on windows system

   # add jhlabs to include
   INCLUDEPATH += ./jhlabs_filter/image
} else:win32 {

    contains(QMAKE_HOST.arch,x86_64) {
        DEFINES+=Q_OS_WIN64
        message("x86_64 build")
        #INCLUDEPATH += "f:/src/ffmpeg_2.8.0/include"
        #LIBS        += -L"f:/src/ffmpeg_2.8.0/lib"
        #INCLUDEPATH += "d:/ffmpeg_build/local64/bin-video/ffmpegSHARED/include"
        #LIBS        += -L"d:/ffmpeg_build/local64/bin-video/ffmpegSHARED/bin"
        #INCLUDEPATH += "f:/msvc/include"
        #LIBS        += -L"f:/msvc/lib/x64"
        #INCLUDEPATH += "d:/ffmpeg_laptop/include"
        #LIBS        += -L"d:/ffmpeg_laptop"
        #INCLUDEPATH += "d:/ffmpeg_build/local64/bin-video/ffmpegSHARED/include"
        #LIBS += -L"d:/ffmpeg_build/local64/bin-video/ffmpegSHARED/bin"
        #INCLUDEPATH += "d:/ffmpeg_build_3_0/local64/bin-video/ffmpegSHARED/include"
        #LIBS += -L"d:/ffmpeg_build_3_0/local64/bin-video/ffmpegSHARED/bin"
        
        #INCLUDEPATH += "f:/ffmpeg/include"
        #LIBS        += -L"f:/ffmpeg/lib"

        #INCLUDEPATH += "f:/ffmpeg.4.0/include"
        #LIBS        += -L"f:/ffmpeg.4.0/lib"

        #INCLUDEPATH += "d:/ffmpeg/ffmpeg.6.0/include"
        #LIBS        += -L"d:/ffmpeg/ffmpeg.6.0/lib"

         INCLUDEPATH += "d:/ffmpeg/ffmpeg.7.0/include"
         LIBS        += -L"d:/ffmpeg/ffmpeg.7.0/lib"
        #INCLUDEPATH += f:/ffmpeg_vs_build_2/msvc/include
        #LIBS+=-L"f:/ffmpeg_vs_build_2/msvc/lib/x64"
    }
    CONFIG(debug, debug|release) {
      LIBS += -L"../ffDiaporama_lib/debug"
    } else {
      LIBS += -L"../ffDiaporama_lib/release"
    }

    CONFIG      += sql                                                      #------ I don't know why, but windows version need sql module in config directive
    DEFINES     += USELIBSWRESAMPLE

    INCLUDEPATH += .                                                        #------ I don't know why, but windows need this !
    INCLUDEPATH += ../exiv2/include/exiv2
    INCLUDEPATH += ../exiv2/include
    LIBS += -L$$top_srcdir/src/exiv2/lib

    #INCLUDEPATH += "../../../win_src/SDL-1.2.15/include"
    INCLUDEPATH += "../../../win_src/msinttypes"

    LIBS        += -lgdi32 -lkernel32 -luser32 -lshell32 -ladvapi32         #------ Windows GDI libs link
    LIBS        += -lswresample                                             #------ conditionnaly include libswresample

    #exiv2 in separate Library
    CONFIG(debug, debug|release) {
       LIBS += -L"../exiv2/debug"
    } else {
       LIBS += -L"../exiv2/release"
    }
    LIBS        += -lexiv2

    RC_FILE     += ffDiaporama.rc
    OTHER_FILES += ffdiaporama.ico \    # icon file to be install on windows system
                   ffDiaporama.url      # URL file to be install on windows system

    LIBS      += -lavformat -lavcodec -lavutil -lswscale -lavfilter -lswresample
}

#--------------------------------------------------------------
# PROJECT FILES
#--------------------------------------------------------------

# Ressource files
RESOURCES   += RSCffDiaporama.qrc

# Source files
SOURCES +=  \
            MainWindow/mainwindow.cpp \
            MainWindow/cCustomSlideTable.cpp \
            DlgRenderVideo/DlgRenderVideo.cpp \
            DlgManageStyle/DlgManageStyle.cpp \
            DlgAbout/DlgAbout.cpp \
            DlgTransition/DlgTransitionProperties.cpp \
            DlgMusic/DlgMusicProperties.cpp \
            DlgMusic/DlgEditMusic.cpp \
            DlgMusic/DlgEditMusicNew.cpp \
            DlgMusic/DlgAdjustToSound.cpp \
            DlgBackground/DlgBackgroundProperties.cpp \
            DlgAppSettings/DlgManageDevices/DlgManageDevices.cpp \
            DlgAppSettings/DlgApplicationSettings.cpp \
            DlgGMapsLocation/DlgGMapsLocation.cpp \
            DlgGMapsLocation/DlgGMapsGeneration.cpp \
            DlgImage/wgt_QGMapsMap/wgt_QGMapsMap.cpp \
            DlgImage/wgt_QGMapsMap/cCustomLocationTable.cpp \
            DlgImage/wgt_QEditImage/cImgInteractiveZone.cpp \
            DlgImage/wgt_QEditImage/wgt_QEditImage.cpp \
            DlgImage/wgt_QEditVideo/wgt_QEditVideo.cpp \
            DlgImage/DlgImageCorrection.cpp \
            DlgSlide/cCustomBlockTable.cpp \
            DlgSlide/DlgSlideProperties.cpp \
            DlgSlide/cInteractiveZone.cpp \
            DlgSlide/cCustomShotTable.cpp \
            DlgSlide/DlgRuler/DlgRulerDef.cpp \
            DlgSlide/cShotComposer.cpp \
            DlgSlide/DlgSlideDuration.cpp \
            DlgSlide/DlgImageComposer.cpp \
            DlgText/cCustomTextEdit.cpp \
            DlgText/DlgTextEdit.cpp \
            DlgCheckConfig/DlgCheckConfig.cpp \
            DlgInfoFile/DlgInfoFile.cpp \
            DlgffDPjrProperties/DlgffDPjrProperties.cpp \
            DlgManageFavorite/DlgManageFavorite.cpp \
            DlgWorkingTask/DlgWorkingTask.cpp \
            DlgTransition/DlgTransitionDuration.cpp \
            DlgFileExplorer/DlgFileExplorer.cpp \
            DlgFileExplorer/FFDFileExplorer.cpp \
            DlgChapter/DlgChapter.cpp \
            DlgAutoTitleSlide/cCustomTitleModelTable.cpp \
            DlgAutoTitleSlide/DlgAutoTitleSlide.cpp \
            DlgExportProject/DlgExportProject.cpp \
            HelpPopup/HelpPopup.cpp \
            HelpPopup/HelpBrowser.cpp \
            engine/_GlobalDefines.cpp \
            engine/cApplicationConfig.cpp \
            engine/cDeviceModelDef.cpp \
            engine/cSoundBlockList.cpp \
            engine/cBaseMediaFile.cpp \
            engine/cVideoFile.cpp \
            engine/cBrushDefinition.cpp \
            engine/cDriveList.cpp \
            engine/ffTools.cpp \
            engine/_Transition.cpp \
            engine/_EncodeVideo.cpp \
            engine/_StyleDefinitions.cpp \
            engine/_Diaporama.cpp \
            engine/_Variables.cpp \
            engine/_Model.cpp \
            engine/cLocation.cpp \
            jhlabs_filter/image/ImageMath.cpp \
            CustomCtrl/_QCustomDialog.cpp \
            CustomCtrl/cCFramingComboBox.cpp \
            CustomCtrl/cCShapeComboBox.cpp \
            CustomCtrl/cThumbnailComboBox.cpp \
            CustomCtrl/cQDateTimeEdit.cpp \
            CustomCtrl/QCustomRuler.cpp \
            CustomCtrl/QMovieLabel.cpp \
            wgt_QMultimediaBrowser/QCustomFolderTable.cpp \
            wgt_QMultimediaBrowser/QCustomFolderTree.cpp \
            wgt_QMultimediaBrowser/wgt_QMultimediaBrowser.cpp \
            wgt_QMultimediaBrowser/FFDFolderTable.cpp \
            wgt_QMultimediaBrowser/FFDFolderTree.cpp \
            wgt_QMultimediaBrowser/FFDMultimediaBrowser.cpp \
            wgt_QVideoPlayer/wgt_QVideoPlayer.cpp \
            HelpPopup/HelpContent.cpp \
            main.cpp

#            engine/qt_cuda.cpp \
#            engine/ffDEncoder.cpp \

# Header files
HEADERS  += MainWindow/cCustomSlideTable.h \
            MainWindow/mainwindow.h \
            DlgRenderVideo/DlgRenderVideo.h \
            DlgManageStyle/DlgManageStyle.h \
            DlgAbout/DlgAbout.h \
            DlgTransition/DlgTransitionProperties.h \
            DlgMusic/DlgMusicProperties.h \
            DlgMusic/DlgEditMusic.h \
            DlgMusic/DlgEditMusicNew.h \
            DlgMusic/DlgAdjustToSound.h \
            DlgBackground/DlgBackgroundProperties.h \
            DlgAppSettings/DlgManageDevices/DlgManageDevices.h \
            DlgAppSettings/DlgApplicationSettings.h \
            DlgGMapsLocation/DlgGMapsLocation.h \
            DlgGMapsLocation/DlgGMapsGeneration.h \
            DlgImage/wgt_QGMapsMap/wgt_QGMapsMap.h \
            DlgImage/wgt_QGMapsMap/cCustomLocationTable.h \
            DlgImage/wgt_QEditImage/cImgInteractiveZone.h \
            DlgImage/wgt_QEditImage/wgt_QEditImage.h \
            DlgImage/wgt_QEditVideo/wgt_QEditVideo.h \
            DlgImage/DlgImageCorrection.h \
            DlgSlide/DlgSlideProperties.h \
            DlgSlide/cCustomBlockTable.h \
            DlgSlide/cInteractiveZone.h \
            DlgSlide/cCustomShotTable.h \
            DlgSlide/DlgRuler/DlgRulerDef.h \
            DlgSlide/cShotComposer.h \
            DlgSlide/DlgSlideDuration.h \
            DlgSlide/DlgImageComposer.h \
            DlgText/cCustomTextEdit.h \
            DlgText/DlgTextEdit.h \
            DlgCheckConfig/DlgCheckConfig.h \
            DlgInfoFile/DlgInfoFile.h \
            DlgffDPjrProperties/DlgffDPjrProperties.h \
            DlgManageFavorite/DlgManageFavorite.h \
            DlgWorkingTask/DlgWorkingTask.h \
            DlgTransition/DlgTransitionDuration.h \
            DlgFileExplorer/DlgFileExplorer.h \
            DlgFileExplorer/FFDFileExplorer.h \
            DlgChapter/DlgChapter.h \
            DlgAutoTitleSlide/cCustomTitleModelTable.h \
            DlgAutoTitleSlide/DlgAutoTitleSlide.h \
            DlgExportProject/DlgExportProject.h \
            HelpPopup/HelpPopup.h \
            HelpPopup/HelpBrowser.h \
            engine/cApplicationConfig.h \
            engine/cDeviceModelDef.h \
            engine/_GlobalDefines.h \
            engine/cSoundBlockList.h \
            engine/cBaseMediaFile.h \
            engine/cVideoFile.h \
            engine/cBrushDefinition.h \
            engine/cDriveList.h \
            engine/ffTools.h \
            engine/_Transition.h \
            engine/_EncodeVideo.h \
            engine/_StyleDefinitions.h \
            engine/_Diaporama.h \
            engine/_Variables.h \
            engine/_Model.h \
            engine/cLocation.h \
            jhlabs_filter/image/ImageMath.h \
            CustomCtrl/_QCustomDialog.h \
            CustomCtrl/cCFramingComboBox.h \
            CustomCtrl/cCShapeComboBox.h \
            CustomCtrl/cThumbnailComboBox.h \
            CustomCtrl/cQDateTimeEdit.h \
            CustomCtrl/QCustomRuler.h \
            CustomCtrl/QMovieLabel.h \
            wgt_QMultimediaBrowser/QCustomFolderTable.h \
            wgt_QMultimediaBrowser/QCustomFolderTree.h \
            wgt_QMultimediaBrowser/wgt_QMultimediaBrowser.h \
            wgt_QMultimediaBrowser/FFDFolderTable.h \
            wgt_QMultimediaBrowser/FFDFolderTree.h \
            wgt_QMultimediaBrowser/FFDMultimediaBrowser.h \
            wgt_QVideoPlayer/wgt_QVideoPlayer.h \
            HelpPopup/HelpContent.h
#            engine/qt_cuda.h \
#            engine/ffDEncoder.h \

# Forms files
FORMS    += MainWindow/mainwindow.ui \
            DlgRenderVideo/DlgRenderVideo.ui \
            DlgManageStyle/DlgManageStyle.ui \
            DlgAbout/DlgAbout.ui \
            DlgTransition/DlgTransitionProperties.ui \
            DlgMusic/DlgMusicProperties.ui \
            DlgMusic/DlgEditMusic.ui \
            DlgBackground/DlgBackgroundProperties.ui \
            DlgAppSettings/DlgManageDevices/DlgManageDevices.ui \
            DlgAppSettings/DlgApplicationSettings.ui \
            DlgGMapsLocation/DlgGMapsLocation.ui \
            DlgGMapsLocation/DlgGMapsGeneration.ui \
            DlgImage/wgt_QGMapsMap/wgt_QGMapsMap.ui \
            DlgImage/wgt_QEditImage/wgt_QEditImageimage.ui \
            DlgImage/wgt_QEditVideo/wgt_QEditVideo.ui \
            DlgImage/DlgImageCorrection.ui \
            DlgSlide/DlgSlideProperties.ui \
            DlgSlide/DlgRuler/DlgRulerDef.ui \
            DlgSlide/DlgImageComposer.ui \
            DlgText/DlgTextEdit.ui \
            DlgCheckConfig/DlgCheckConfig.ui \
            DlgInfoFile/DlgInfoFile.ui \
            DlgffDPjrProperties/DlgffDPjrProperties.ui \ 
            DlgManageFavorite/DlgManageFavorite.ui \
            DlgWorkingTask/DlgWorkingTask.ui \
            DlgTransition/DlgTransitionDuration.ui \
            DlgSlide/DlgSlideDuration.ui \
            DlgFileExplorer/DlgFileExplorer.ui \
            DlgFileExplorer/FFDFileExplorer.ui \
            DlgChapter/DlgChapter.ui \
            DlgAutoTitleSlide/DlgAutoTitleSlide.ui \
            DlgExportProject/DlgExportProject.ui \
            wgt_QMultimediaBrowser/wgt_QMultimediaBrowser.ui \
            wgt_QMultimediaBrowser/FFDMultimediaBrowser.ui \
            wgt_QVideoPlayer/wgt_QVideoPlayer.ui \
            HelpPopup/HelpPopup.ui \
            DlgMusic/DlgAdjustToSound.ui

# Translation files
message("xpro top_srcdir is "$$top_srcdir)
TRANSLATIONS += $$top_srcdir/locale/ffDiaporama_cz.ts \
    $$top_srcdir/locale/ffDiaporama_de.ts \
    $$top_srcdir/locale/ffDiaporama_el.ts \
    $$top_srcdir/locale/ffDiaporama_es.ts \
    $$top_srcdir/locale/ffDiaporama_fr.ts \
    $$top_srcdir/locale/ffDiaporama_gl.ts \
    $$top_srcdir/locale/ffDiaporama_it.ts \
    $$top_srcdir/locale/ffDiaporama_nl.ts \
    $$top_srcdir/locale/ffDiaporama_pt.ts \
    $$top_srcdir/locale/ffDiaporama_ru.ts \
    $$top_srcdir/locale/ffDiaporama_zh_tw.ts

#TRANSLATIONS += ../../locale/ffDiaporama_cz.ts \
#    ../../locale/ffDiaporama_de.ts \
#    ../../locale/ffDiaporama_el.ts \
#    ../../locale/ffDiaporama_es.ts \
#    ../../locale/ffDiaporama_fr.ts \
#    ../../locale/ffDiaporama_gl.ts \
#    ../../locale/ffDiaporama_it.ts \
#    ../../locale/ffDiaporama_nl.ts \
#    ../../locale/ffDiaporama_pt.ts \
#    ../../locale/ffDiaporama_ru.ts \
#    ../../locale/ffDiaporama_zh_tw.ts

#--------------------------------------------------------------
# INSTALLATION
#--------------------------------------------------------------
TARGET.path         = $$PREFIX/bin
TARGET.files        = $$TARGET
INSTALLS 	    += TARGET

Licences.path       = $$PREFIX/share/$$APPFOLDER
Licences.files      = ../../authors.txt \
                      ../../licences.txt \
                      ../../licence.rtf
INSTALLS            += Licences

XMLConfig.path      = $$PREFIX/share/$$APPFOLDER
XMLConfig.files     = ../../Devices.xml \
                      ../../ffDiaporama.xml
INSTALLS            += XMLConfig

General.path        = $$PREFIX/share/$$APPFOLDER
General.files       = ../../changelog-en.txt \
                      ../../changelog-fr.txt \
                      ../../BUILDVERSION.txt \
                      ../../readme.txt
INSTALLS            += General

ico.path            = $$PREFIX/share/icons/hicolor/32x32/apps
ico.files           = ../../ffdiaporama.png
INSTALLS 	    += ico

desktop.path        = $$PREFIX/share/applications
desktop.files       = ../../ffDiaporama.desktop
INSTALLS 	    += desktop

mimefile.path       = $$PREFIX/share/mime/packages
mimefile.files      = ../../ffDiaporama-mime.xml
INSTALLS 	    += mimefile

translation.path    = $$PREFIX/share/$$APPFOLDER/locale
translation.files   = ../../locale/wiki_en.*
INSTALLS 	    += translation

win32 {
   contains(QMAKE_HOST.arch,x86_64) {
   	CONFIG(debug, debug|release)  {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama/debug
   	}
	   CONFIG(release, debug|release) {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama/release
      }
   }
   else {
   	CONFIG(debug, debug|release)  {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama_x86/debug
   	}
	   CONFIG(release, debug|release) {
                OBJECTS_DIR_BASE = d:/obj/ffdiporama_x86/release
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
    # should not work withdrive-letters here...:(
   DESTDIR=d:/ffDiaporama
   contains(QMAKE_HOST.arch,x86_64) {
      DESTDIR=d:/ffDiaporama_x64
   }
   !debug_and_release|build_pass {
	    CONFIG(debug, debug|release) {
          TARGET = $$member(TARGET, 0)d
          !isEmpty(QMAKE_TARGET_DESCRIPTION){
				   QMAKE_TARGET_DESCRIPTION = $$member(QMAKE_TARGET_DESCRIPTION,0) (DEBUG-Version)
          }
	   }
   }
   PRECOMPILED_HEADER = ffdiaporama_inc.h
   QMAKE_LFLAGS_RELEASE += /OPT:NOREF
}
   # for hevc/h265 and VP9
   DEFINES += ENABLE_XCODECS

transl.path    = $$PREFIX/share/$$APPFOLDER/locale
transl.files   = ../../locale/*.qm
INSTALLS 	    += transl
message("transl.path $$transl.path")
message("transl.files $$transl.files")
message("INSTALLS $$INSTALLS")

# cuda-part
#INCLUDEPATH += $(CUDA_INC_PATH)
#LIBS += -L$(CUDA_LIB_PATH)
#LIBS += -lcudart_static -lnppi -lnppc 



# Cuda extra-compiler for handling files specified in the CUSOURCES variable
#
QMAKE_CUC = $(CUDA_BIN_PATH)/nvcc.exe
{
	cu.name = Cuda ${QMAKE_FILE_IN}
	cu.input = CUSOURCES
   cu.depends = $$CUSOURCES
	cu.CONFIG += no_link
	cu.variable_out = OBJECTS
	isEmpty(QMAKE_CUC) {
      win32:QMAKE_CUC = $(CUDA_BIN_PATH)/nvcc.exe
      else:QMAKE_CUC = nvcc
	}
   SOURCES += $$CUSOURCES

	isEmpty(CU_DIR):CU_DIR = .
	isEmpty(QMAKE_CPP_MOD_CU):QMAKE_CPP_MOD_CU = 
	isEmpty(QMAKE_EXT_CPP_CU):QMAKE_EXT_CPP_CU = .cu
        !isEmpty(CUDA_INC_PATH):INCLUDEPATH += $(CUDA_INC_PATH)

   CONFIG(debug, debug|release) {
      QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_DEBUG $$QMAKE_CXXFLAGS_RTTI_ON $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_STL_ON
   } 	
   CONFIG(release, debug|release) {
      QMAKE_CUFLAGS += $$QMAKE_CXXFLAGS $$QMAKE_CXXFLAGS_RELEASE $$QMAKE_CXXFLAGS_RTTI_ON $$QMAKE_CXXFLAGS_WARN_ON $$QMAKE_CXXFLAGS_STL_ON
   } 	
	QMAKE_CUEXTRAFLAGS += -Xcompiler $$join(QMAKE_CUFLAGS, ",")
   !contains(QMAKE_HOST.arch, x86_64) {
      ## Windows x86 (32bit) specific build here
      QMAKE_CUEXTRAFLAGS += --machine 32 --debug
   } else {
      ## Windows x64 (64bit) specific build here
      QMAKE_CUEXTRAFLAGS += --machine 64
   }	
   QMAKE_CUEXTRAFLAGS += $(DEFINES) $(INCPATH) $$join(QMAKE_COMPILER_DEFINES, " -D", -D)
	QMAKE_CUEXTRAFLAGS += -c


   cu.dependency_type = TYPE_C 
	cu.commands = \"$$QMAKE_CUC\" $$QMAKE_CUEXTRAFLAGS -o $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ} ${QMAKE_FILE_NAME}$$escape_expand(\n\t)
	cu.output = $$OBJECTS_DIR/$${QMAKE_CPP_MOD_CU}${QMAKE_FILE_BASE}$${QMAKE_EXT_OBJ}
	silent:cu.commands = @echo nvcc ${QMAKE_FILE_IN} && $$cu.commands
	QMAKE_EXTRA_COMPILERS += cu

	build_pass|isEmpty(BUILDS):cuclean.depends = compiler_cu_clean
	else:cuclean.CONFIG += recursive
	QMAKE_EXTRA_TARGETS += cuclean
}


