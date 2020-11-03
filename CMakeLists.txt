cmake_minimum_required(VERSION 3.0)


# Notre projet est étiqueté hello
project( StationairAnnotate )


find_package( OpenCV REQUIRED )

find_package(Qt5 5.7.0 REQUIRED COMPONENTS
    Core
)

find_package(Qt5 COMPONENTS Gui REQUIRED)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS PrintSupport REQUIRED)
find_package(Qt5 COMPONENTS Network REQUIRED)


set(CMAKE_AUTOMOC ON)

# Crée des variables avec les fichiers à compiler
set(SRCS
    main.cpp
    AnnotateArea.cpp
    AnnotationsSet.cpp
    NetworkHandler.cpp
    DialogClassSelection.cpp
    AnnotationsBrowser.cpp
    MainWindow.cpp
    ParamsQEditorWindow.cpp
    ParamsQEditorLine.cpp
    SuperPixelsAnnotate.cpp
    OptFlowTracking.cpp
    )
    
set(HEADERS
    AnnotateArea.h
    AnnotationsSet.h
    NetworkHandler.h
    MainWindow.h
    DialogClassSelection.h
    QtCvUtils.h
    AnnotationsBrowser.h
    ParamsHandler.h
    ParamsQEditorLine.h
    ParamsQEditorWindow.h
    SuperPixelsAnnotate.h
    OptFlowTracking.h
    )


add_executable( StationairAnnotate ${SRCS} ${HEADERS} )

target_link_libraries( StationairAnnotate ${OpenCV_LIBS} Qt5::Core Qt5::Gui Qt5::Widgets Qt5::PrintSupport Qt5::Network )

# Cette ligne doit être placée après les add_executable/add_library
target_compile_features(StationairAnnotate PUBLIC cxx_nullptr)

