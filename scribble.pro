QT += widgets
requires(qtConfig(filedialog))
qtHaveModule(printsupport): QT += printsupport







# OpenCV specific stuff.... also :
# in the project / build / build environment thing, add /usr/local/bin to PATH
# and add a PKG_CONFIG_PATH entry with value /usr/local/Cellar/opencv/3.4.0_1/lib/pkgconfig/opencv.pc
# and in the project / run / run environment section, remove the DYLD_LIBRARY_PATH value (under el capitan)
#QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv








HEADERS       = \
    AnnotateArea.h \
    AnnotationsSet.h \
    MainWindow.h \
    DialogClassSelection.h \
    QtCvUtils.h \
    AnnotationsBrowser.h
SOURCES       = main.cpp \
    AnnotateArea.cpp \
    AnnotationsSet.cpp \
    DialogClassSelection.cpp \
    AnnotationsBrowser.cpp \
    MainWindow.cpp

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/widgets/scribble
INSTALLS += target
