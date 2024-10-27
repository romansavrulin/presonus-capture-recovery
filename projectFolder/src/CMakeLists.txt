# Add here any file that you want to be part of the library target,
# but it is not in a projectFolder/src/dir directory
set(sources_list
    # anotherFolder/myfile.cpp
)

# Get all the .cpp files in any projectFolder/src/dir directory
file(GLOB directories */)
foreach(dir ${directories})
    if(IS_DIRECTORY ${dir})
        string(FIND ${dir} "/" last_slash_pos REVERSE)
        math(EXPR string_start "${last_slash_pos}+1")
        string(SUBSTRING ${dir} ${string_start} -1 dir_stripped)
        file(GLOB_RECURSE sources ${dir_stripped}/*.cpp)
        list(APPEND sources_list ${sources})
    endif()
endforeach()

# Create a library target that contains all of the sources
add_library(${PROJECT_NAME}_LIB ${sources_list})

# Add the include directories to the library target.
# Remember to add any include that is not in a projectFolder/src/dir directory
target_include_directories(
    ${PROJECT_NAME}_LIB
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third-party/
)

# Define the compiler flags based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMPILER_FLAGS
        -Wall
        -O0
    )
    if(UNIX)
        list(APPEND COMPILER_FLAGS
            #-Werror
            -Wpedantic
            -Wextra
        )
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(COMPILER_FLAGS
        -DNDEBUG
    )
    if(UNIX)
        list(APPEND COMPILER_FLAGS
            -O3
            -march=native
            -ffast-math
        )
    endif()
endif()

# Set the compiler flags for the library target
target_compile_options(
    ${PROJECT_NAME}_LIB
    PUBLIC
        ${COMPILER_FLAGS}
)

# Define the preprocessor macros based on the build type.
# This is equivalent to using #define in the source code
# or using -D flags as a compiler option
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PREPROCESSOR_MACROS
        DEBUGG
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(PREPROCESSOR_MACROS
        RELEASE
    )
endif()

if(LINUX)
    list(APPEND PREPROCESSOR_MACROS
        LINUX
    )
elseif(APPLE)
    list(APPEND PREPROCESSOR_MACROS
        MACOS
    )
elseif(WIN32)
    list(APPEND PREPROCESSOR_MACROS
        WINDOWS
    )
endif()

# Set the preprocessor macros for the library target
target_compile_definitions(
    ${PROJECT_NAME}_LIB
    PUBLIC
        ${PREPROCESSOR_MACROS}
)

# Set the C++ language features for the library target
target_compile_features(
    ${PROJECT_NAME}_LIB
    PUBLIC
        c_std_11
        cxx_std_20
)

# Ensure that the C++ standard is mandatory
# and disable compiler-specific extensions
set_target_properties(
    ${PROJECT_NAME}_LIB
        PROPERTIES
            CXX_STANDARD_REQUIRED ON
            CXX_EXTENSIONS OFF
)

find_package(Boost 1.85 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Jsoncpp REQUIRED)
include_directories(${Jsoncpp_INCLUDE_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

if(Jsoncpp_FOUND)
    message("INFO: we found LibJsoncpp")
    message(Jsoncpp_FOUND = ${Jsoncpp_FOUND})
    message(Jsoncpp_INCLUDE_DIR = ${JSONCPP_INCLUDE_DIRS})
    message(JSONCPP_LINK_LIBRARIES = ${JSONCPP_LINK_LIBRARIES})
else(Jsoncpp_FOUND)
    message("WARNING: we couldn't find libjsoncpp-dev")
endif(Jsoncpp_FOUND)

# Define the linker options based on the build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LINKER_OPTIONS
    )
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(LINKER_OPTIONS
    )
    if(UNIX)
        list(APPEND LINKER_OPTIONS
            -flto           # Enable link-time optimizations
        )
    endif()
    if(LINUX)
        list(APPEND LINKER_OPTIONS
            -Wl,--strip-all # Strip debug symbols
        )
    endif()
endif()

if(LINUX)
    list(APPEND LINKER_OPTIONS
        -Wl,--no-undefined # Fail if there are any unresolved symbols in the final linked output
    )
endif()

# Set the linker options for the library target
target_link_options(
    ${PROJECT_NAME}_LIB
    PUBLIC
        ${LINKER_OPTIONS}
)

target_link_libraries(
    ${PROJECT_NAME}_LIB
    PUBLIC
        # Add libraries to link to the binary here
        Boost::program_options
        ${JSONCPP_LINK_LIBRARIES}
)
