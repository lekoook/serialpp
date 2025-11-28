cmake_minimum_required(VERSION 3.10.0)
project(serialpp VERSION 0.1.0 LANGUAGES C CXX)

# Source files
set(SRCS
    src/serialpp.cpp
)
if(UNIX)
    list(APPEND SRCS src/impl/unix.cpp)
endif()

# Library
add_library(serialpp ${SRCS})
target_include_directories(serialpp PRIVATE src)
target_compile_features(serialpp PUBLIC cxx_std_17)

# Examples
add_executable(list_ports_eg examples/list_ports/list_ports.cpp)
target_include_directories(list_ports_eg PRIVATE src)
target_compile_features(serialpp PUBLIC cxx_std_17)
target_link_libraries(list_ports_eg serialpp)

add_executable(read_write_eg examples/read_write/read_write.cpp)
target_include_directories(read_write_eg PRIVATE src)
target_compile_features(read_write_eg PUBLIC cxx_std_17)
target_link_libraries(read_write_eg serialpp)

add_executable(async_read_write_eg examples/async_read_write/async_read_write.cpp)
target_include_directories(async_read_write_eg PRIVATE src)
target_compile_features(async_read_write_eg PUBLIC cxx_std_17)
target_link_libraries(async_read_write_eg serialpp)

add_executable(subscription_eg examples/subscription/subscription.cpp)
target_include_directories(subscription_eg PRIVATE src)
target_compile_features(subscription_eg PUBLIC cxx_std_17)
target_link_libraries(subscription_eg serialpp)

# Build doxygen docs
find_package(Doxygen REQUIRED COMPONENTS dot)
if (Doxygen_FOUND)
    doxygen_add_docs(doc
        ${PROJECT_SOURCE_DIR}
        ALL
        COMMENT "Generating API documentation with Doxygen"
        CONFIG_FILE ${PROJECT_SOURCE_DIR}/Doxyfile
    )
else()
    message(STATUS "Doxygen not found, documentation will not be generated.")
endif()

