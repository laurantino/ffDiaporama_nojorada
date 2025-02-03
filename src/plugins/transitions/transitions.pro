TEMPLATE      = lib
CONFIG       += plugin
QT           += widgets
INCLUDEPATH  += ../../ffDiaporama
HEADERS       = transitionsplugin.h
SOURCES       = transitionsplugin.cpp
TARGET        = $$qtLibraryTarget(transitions)
#DESTDIR       = ../../plugins
#DESTDIR=f:/ffDiaporama_x64/plugins
CONFIG(debug, debug|release) {
   DESTDIR=/home/norbert/bin/ffDiaporama
}
CONFIG(relase, debug|release) {
   DESTDIR=/home/norbert/bin/ffDiaporama
}
# install
#target.path = f:/ffDiaporama_x64/plugins
INSTALLS += target

CONFIG += install_ok  # Do not cargo-cult this!
uikit: CONFIG += debug_and_release
