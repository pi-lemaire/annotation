QT += widgets
requires(qtConfig(filedialog))
qtHaveModule(printsupport): QT += printsupport




# OpenCV specific stuff.... also :
# in the project / build / build environment thing, add /usr/local/bin to PATH
# and add a PKG_CONFIG_PATH entry with value /usr/local/Cellar/opencv/4.2.0_1/lib/pkgconfig/opencv4.pc
# and in the project / run / run environment section, remove the DYLD_LIBRARY_PATH value (under el capitan)
#QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv4
#PKGCONFIG += opencv


#INCLUDEPATH += /usr/local/opt/opencv/include/opencv4




# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#SOURCES += main.cpp



#QT += widgets
QT += network






HEADERS       = \
    AnnotateArea.h \
    AnnotationsSet.h \
    MainWindow.h \
    DialogClassSelection.h \
    NetworkHandler.h \
    QtCvUtils.h \
    AnnotationsBrowser.h \
    ParamsHandler.h \
    ParamsQEditorLine.h \
    ParamsQEditorWindow.h \
    SuperPixelsAnnotate.h \
    OptFlowTracking.h
SOURCES       = main.cpp \
    AnnotateArea.cpp \
    AnnotationsSet.cpp \
    DialogClassSelection.cpp \
    AnnotationsBrowser.cpp \
    MainWindow.cpp \
    NetworkHandler.cpp \
    ParamsQEditorWindow.cpp \
    ParamsQEditorLine.cpp \
    SuperPixelsAnnotate.cpp \
    OptFlowTracking.cpp

# install
#target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/scribble
#INSTALLS += target
