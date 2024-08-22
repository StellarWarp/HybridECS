

set(CURRENT_PROJECT_NAME HYECS)

set(CMAKE_CXX_STANDARD 23)
# src at ./src
# recursive add all files in src
set(SRC_FOLDERS "${CMAKE_CURRENT_SOURCE_DIR}/src")
#file(GLOB_RECURSE SRC_FILES "${SRC_FOLDERS}/*.cpp" "${SRC_FOLDERS}/*.h" "${SRC_FOLDERS}/*.hpp")

add_executable(${CURRENT_PROJECT_NAME} ${SRC_FOLDERS}/HybridECS.cpp)


# pch file is at ./src/pch.h
# pch compile unit is at ./src/pch.cpp

target_precompile_headers(${CURRENT_PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${SRC_FOLDERS}/pch.h>")

# include ./src
target_include_directories(${CURRENT_PROJECT_NAME} PRIVATE src)

# add all files in src to the project



