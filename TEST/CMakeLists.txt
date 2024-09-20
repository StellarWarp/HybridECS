set(CURRENT_PROJECT_NAME HYECS_TEST)

set(CMAKE_CXX_STANDARD 23)
# src at ./src
# recursive add all files in src
set(SRC_FOLDERS "${CMAKE_CURRENT_SOURCE_DIR}/src")
#file(GLOB_RECURSE SRC_FILES "${SRC_FOLDERS}/*.cpp" "${SRC_FOLDERS}/*.h" "${SRC_FOLDERS}/*.hpp")

file(GLOB_RECURSE SRC_FILES "${SRC_FOLDERS}/*.cpp")
add_executable(${CURRENT_PROJECT_NAME} ${SRC_FILES})

include_directories("src")

