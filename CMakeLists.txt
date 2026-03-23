cmake_minimum_required(VERSION 3.16)
project(elcmd VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Test)

set(THREADS_PREFER_PTHREAD_FLAG ON)

# Core library for reuse by tests and app
add_library(core STATIC
    src/FileModel.cpp
    src/FileService.cpp
    src/ArchiveService.cpp
    src/Logger.cpp
)

add_executable(elcmd
    src/main.cpp
    src/PaneWidget.cpp
    src/ArchiveViewerDialog.cpp
    src/ConflictDialog.cpp
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(core PUBLIC Qt6::Core Qt6::Concurrent Qt6::Widgets)

# Optional: link with libarchive if available for robust archive handling
find_library(LIBARCHIVE_LIBRARY NAMES archive libarchive)
if(LIBARCHIVE_LIBRARY)
    message(STATUS "Found libarchive: ${LIBARCHIVE_LIBRARY}")
    target_link_libraries(core PUBLIC ${LIBARCHIVE_LIBRARY})
    target_compile_definitions(core PUBLIC HAVE_LIBARCHIVE)
endif()


target_link_libraries(elcmd PRIVATE Qt6::Widgets core)

# Tests
enable_testing()
add_executable(test_filemodel tests/TestFileModel.cpp)
target_link_libraries(test_filemodel PRIVATE Qt6::Test core)
add_test(NAME filemodel_test COMMAND test_filemodel)

add_executable(test_fileservice tests/TestFileService.cpp)
target_link_libraries(test_fileservice PRIVATE Qt6::Test core)
add_test(NAME fileservice_test COMMAND test_fileservice)

add_executable(test_paneops tests/TestPaneOps.cpp src/PaneWidget.cpp src/ArchiveViewerDialog.cpp src/ConflictDialog.cpp)
target_link_libraries(test_paneops PRIVATE Qt6::Test Qt6::Widgets core)
add_test(NAME paneops_test COMMAND test_paneops)

add_executable(test_rightclick tests/TestRightClick.cpp src/PaneWidget.cpp src/ArchiveViewerDialog.cpp src/ConflictDialog.cpp)
target_link_libraries(test_rightclick PRIVATE Qt6::Test Qt6::Widgets core)
add_test(NAME rightclick_test COMMAND test_rightclick)

# Installation and packaging (CPack)
install(TARGETS elcmd RUNTIME DESTINATION bin)
install(TARGETS core ARCHIVE DESTINATION lib)
install(DIRECTORY src/ DESTINATION share/elcmd)

set(CPACK_PACKAGE_NAME "elcmd")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "TGZ")
include(CPack)
