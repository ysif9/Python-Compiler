cmake_minimum_required(VERSION 3.26)

project(Python_Compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt setup
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Include directories
include_directories(
        include
        GUI/include
)

# Source files
set(SOURCES
        main.cpp

        # GUI
        GUI/mainwindow.cpp
        GUI/codeeditor.cpp
        GUI/linenumberarea.cpp
        GUI/pythonhighlighter.cpp
        GUI/findreplacedialog.cpp
        GUI/symboltabledialog.cpp
        GUI/tokensequencedialog.cpp
        GUI/errordialog.cpp

        # Compiler backend
        Lexer/Lexer.cpp
        Parser/Parser.cpp
        GUI/ThemeUtility.cpp
        GUI/ParserTreeDialog.cpp
        GUI/include/ParserTreeDialog.hpp
)

# Header files (for Qt's MOC)
set(HEADERS
        GUI/include/mainwindow.hpp
        GUI/include/codeeditor.hpp
        GUI/include/linenumberarea.hpp
        GUI/include/pythonhighlighter.hpp
        GUI/include/findreplacedialog.hpp
        GUI/include/symboltabledialog.hpp
        GUI/include/tokensequencedialog.hpp
        GUI/include/errordialog.hpp

        include/Lexer.hpp
        include/Token.hpp
        include/Parser.hpp
        GUI/include/ThemeUtility.hpp
        GUI/ParserTreeDialog.cpp
        GUI/include/ParserTreeDialog.hpp
)

add_executable(Python_Compiler ${SOURCES} ${HEADERS})

target_link_libraries(Python_Compiler PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Optional macOS/iOS settings
set_target_properties(Python_Compiler PROPERTIES
        MACOSX_BUNDLE TRUE
        WIN32_EXECUTABLE TRUE
)

if (QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Python_Compiler)
endif ()
