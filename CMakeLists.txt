cmake_minimum_required(VERSION 3.16)

project(Search_System VERSION 0.2 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Concurrent)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    structs.h
    connectiondialog.cpp connectiondialog.h connectiondialog.ui
    searchsettings.cpp searchsettings.h
    database.cpp database.h
    indexingdialog.cpp indexingdialog.h indexingdialog.ui
    indexer.cpp indexer.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(Search_System MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    add_executable(Search_System ${PROJECT_SOURCES})
endif()

target_link_libraries(Search_System PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
)

if(WIN32)
    # QPSQL loads libpq dynamically. The repository contains runtime DLLs,
    # not the import libraries required by MSVC/MinGW linkers.
    set(POSTGRES_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/WINSQL_Libs")
    foreach(runtime_dll libpq.dll libcrypto-3-x64.dll libiconv-2.dll libintl-9.dll libssl-3-x64.dll)
        if(EXISTS "${POSTGRES_RUNTIME_DIR}/${runtime_dll}")
            add_custom_command(TARGET Search_System POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${POSTGRES_RUNTIME_DIR}/${runtime_dll}"
                    "$<TARGET_FILE_DIR:Search_System>"
                VERBATIM)
        endif()
    endforeach()
else()
    find_package(PostgreSQL REQUIRED)
    target_link_libraries(Search_System PRIVATE PostgreSQL::PostgreSQL)
endif()

set_target_properties(Search_System PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.example.searchsystem"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Search_System
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Search_System)
endif()
