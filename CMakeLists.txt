# CMakeLists.txt - Version finale nettoyée
cmake_minimum_required(VERSION 3.16)
project(DrumBoxMultiplayer)

# Configuration C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recherche de Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Multimedia)

# Configuration automatique Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Fichiers sources
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/DrumGrid.cpp
    src/AudioEngine.cpp
    src/NetworkManager.cpp
    src/DrumServer.cpp
    src/DrumClient.cpp
    src/Protocol.cpp
    src/Room.cpp
    src/RoomManager.cpp
    src/RoomListWidget.cpp
    src/UserListWidget.cpp
)

# Headers (SANS NetworkClasses.h qui cause des redéfinitions)
set(HEADERS
    include/MainWindow.h
    include/DrumGrid.h
    include/AudioEngine.h
    include/NetworkManager.h
    include/DrumServer.h
    include/DrumClient.h
    include/Protocol.h
    include/Room.h
    include/RoomManager.h
    include/RoomListWidget.h
    include/UserListWidget.h
)

# Exécutable
add_executable(DrumBoxMultiplayer ${SOURCES} ${HEADERS})

# Répertoires d'inclusion
target_include_directories(DrumBoxMultiplayer PRIVATE include)

# Liaison Qt6
target_link_libraries(DrumBoxMultiplayer PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
)

# Configuration Windows
if(WIN32)
    set_target_properties(DrumBoxMultiplayer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Copier le dossier samples si il existe
if(EXISTS "${CMAKE_SOURCE_DIR}/samples")
    file(COPY "${CMAKE_SOURCE_DIR}/samples" DESTINATION "${CMAKE_BINARY_DIR}")
    message(STATUS "Dossier samples copié")
else()
    message(STATUS "Dossier samples non trouvé - mode silencieux activé")
endif()

# Après la ligne qui copie les samples, ajoutez :
if(EXISTS "${CMAKE_SOURCE_DIR}/icons")
    file(COPY "${CMAKE_SOURCE_DIR}/icons" DESTINATION "${CMAKE_BINARY_DIR}")
    message(STATUS "Dossier icons copié")
endif()

# Configuration debug/release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(DrumBoxMultiplayer PRIVATE DEBUG_MODE)
endif()

message(STATUS "Configuration terminée pour Qt6 ${Qt6_VERSION}")

# Compter les fichiers
list(LENGTH SOURCES SOURCES_COUNT)
list(LENGTH HEADERS HEADERS_COUNT)
message(STATUS "Nombre de fichiers sources: ${SOURCES_COUNT}")
message(STATUS "Nombre de fichiers headers: ${HEADERS_COUNT}")

# Informations de compilation
message(STATUS "Configuration CMake:")
message(STATUS "  - Compilateur C++: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  - Standard C++: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Type de build: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Répertoire de build: ${CMAKE_BINARY_DIR}")
message(STATUS "  - Répertoire source: ${CMAKE_SOURCE_DIR}")
