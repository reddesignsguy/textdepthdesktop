

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
        ActionToolBar.qml
        SOURCES TextDepth.h TextDepth.cpp
        SOURCES TextDepthUnit.h TextDepthUnit.cpp
        SOURCES PhotoshopWriter.h
        SOURCES photoshopwriter.h photoshopwriter.cpp
        SOURCES qtToPhotoshopAPI.h
        SOURCES qtToPhotoshopAPI.cpp
        SOURCES LayerUIModel.h LayerUIModel.cpp
        SOURCES TextDepthViewport.h TextDepthViewport.cpp
        SOURCES drawable.h
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

option(TEXTDEPTH_BUILD_TESTS "Build PSD loading regression tests" OFF)
if(TEXTDEPTH_BUILD_TESTS)
    enable_testing()
    qt_add_executable(textdepth_psd_loading_test
        tests/psd_loading.cpp
        TextDepth.h TextDepth.cpp
        TextDepthUnit.h TextDepthUnit.cpp
        PhotoshopWriter.cpp qtToPhotoshopAPI.cpp
    )
    target_include_directories(textdepth_psd_loading_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(textdepth_psd_loading_test PRIVATE Qt6::Quick PhotoshopAPI)
    add_test(NAME psd_loading COMMAND textdepth_psd_loading_test)
endif()
