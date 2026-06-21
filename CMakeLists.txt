

set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architecture") # reddesigns: required on mac
message(STATUS "ARCH: ${CMAKE_OSX_ARCHITECTURES}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

cmake_minimum_required(VERSION 3.16)
project(TextDepthOG VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Quick)


qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appTextDepthOG
    main.cpp
)

qt_add_qml_module(appTextDepthOG
    URI TextDepthOG
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES TextDepth.h TextDepth.cpp
        SOURCES PhotoshopWriter.h
        SOURCES photoshopwriter.h photoshopwriter.cpp
        SOURCES qtToPhotoshopAPI.h
        SOURCES qtToPhotoshopAPI.cpp
        SOURCES LayerUIModel.h LayerUIModel.cpp
        SOURCES TextDepthViewport.h TextDepthViewport.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appTextDepthOG PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appTextDepthOG
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appTextDepthOG
    PRIVATE Qt6::Quick
)

add_subdirectory(PhotoshopAPI)
target_link_libraries(appTextDepthOG PRIVATE PhotoshopAPI)
target_link_libraries(appTextDepthOG PRIVATE Qt6::Core)
target_link_libraries(appTextDepthOG PRIVATE Qt6::Quick)

include(GNUInstallDirs)
install(TARGETS appTextDepthOG
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
