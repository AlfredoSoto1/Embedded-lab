cmake_minimum_required(VERSION 3.10)
project(lab2 C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Find libgpiod library (required for Raspberry Pi GPIO)
find_library(GPIOD_LIBRARY gpiod)
if(NOT GPIOD_LIBRARY)
    message(FATAL_ERROR "libgpiod not found. Install it with: sudo apt-get install libgpiod-dev")
endif()

# Find all C source files recursively in src/ directory
file(GLOB_RECURSE ALL_SOURCES "src/*.c")

# Helper source files (no main function) - add these to executables that need them
set(HELPER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/lcd_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/keyp_api.c
)

# Program source files (have main function)
set(PROGRAM_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM PROGRAM_SOURCES ${HELPER_SOURCES})

# Create executables for each program file
foreach(SOURCE_FILE ${PROGRAM_SOURCES})
    # Get filename without extension
    get_filename_component(EXEC_NAME ${SOURCE_FILE} NAME_WE)
    
    # Create executable (include helper sources if needed)
    add_executable(${EXEC_NAME} ${SOURCE_FILE} ${HELPER_SOURCES})
    
    # Link libraries
    target_link_libraries(${EXEC_NAME} ${GPIOD_LIBRARY} m)
    
    message(STATUS "Added executable: ${EXEC_NAME}")
endforeach()

message(STATUS "Program sources: ${PROGRAM_SOURCES}")
message(STATUS "Helper sources: ${HELPER_SOURCES}")
