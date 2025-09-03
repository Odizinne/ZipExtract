cmake_minimum_required(VERSION 3.16)

project(ZipExtract VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

set(HEADERS
    include/registryhelper.h
    include/zipextractor.h
)

set(SOURCES
    src/registryhelper.cpp
    src/zipextractor.cpp
    src/main.cpp
)

set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/windows_metadata.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
    @ONLY
)

qt_add_executable(${CMAKE_PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    resources/icons/icons.qrc
    "${CMAKE_CURRENT_BINARY_DIR}/windows_metadata.rc"
)

set(QML_FILES
    qml/Main.qml
    qml/Extractor.qml
)

set(QML_SINGLETONS
    #qml/Singletons/Common.qml
)

set_source_files_properties(${QML_SINGLETONS}
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(${CMAKE_PROJECT_NAME}
    URI ${CMAKE_PROJECT_NAME}
    VERSION 1.0
    QML_FILES ${QML_FILES} ${QML_SINGLETONS}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(TARGETS ${CMAKE_PROJECT_NAME}
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET ${CMAKE_PROJECT_NAME}
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types designer,iconengines,qmllint,generic,networkinformation,help,qmltooling,sqldrivers,qmlls
)
install(SCRIPT ${deploy_script})

