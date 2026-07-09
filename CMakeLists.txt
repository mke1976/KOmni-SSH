cmake_minimum_required(VERSION 3.16)
project(komni-ssh VERSION 1.0.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic)

# Find Qt 6 libraries
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

# Enable Qt features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Source files list
set(SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/SettingsDialog.h
    src/SettingsDialog.cpp
    src/ConfigManager.h
    src/ConfigManager.cpp
    src/ConnectionManager.h
    src/ConnectionManager.cpp
    src/resources.qrc
)

# Build target
add_executable(komni-ssh ${SOURCES})

# Link libraries
target_link_libraries(komni-ssh PRIVATE Qt6::Widgets Qt6::Network)

# Installation definitions
include(GNUInstallDirs)
install(TARGETS komni-ssh RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install Desktop file
install(FILES komni-ssh.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Install Application icon
install(FILES src/icons/app_icon.png DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps RENAME komni-ssh.png)
