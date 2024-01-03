cmake_minimum_required(VERSION 3.18)
project(knifer_scalp)

set_property( GLOBAL PROPERTY USE_FOLDERS ON )
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CONAN_DISABLE_CHECK_COMPILER ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)

if(MSVC)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20 /Zc:__cplusplus /MP /Zc:preprocessor" CACHE STRING "compile flags" FORCE)
	# ignore "not found pdb"
	add_link_options("/ignore:4099")
endif()
add_compile_definitions(BX_CONFIG_DEBUG=0)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(src/CMakeLists.txt)
target_include_directories(main PRIVATE src/)
target_link_libraries(main ${CONAN_LIBS} bgfx)

# turn off for debug
# set_property(TARGET main PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
