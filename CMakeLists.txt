cmake_minimum_required(VERSION 3.4)
project(sunrise)

set(SEC_FLAGS "-fstack-protector-strong -D_FORTIFY_SOURCE=2 -fPIE")

if(APPLE) #cry
  set(SEC_LINK_FLAGS "")
else()
  set(SEC_LINK_FLAGS "-Wl,-z,relro,-z,now,-z,noexecstack -pie")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release ... FORCE)
endif()

set(CMAKE_C_COMPILER "clang")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics -Wall -Wextra -pedantic -Wno-unused-parameter -O3 ${SEC_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3") #valgrind
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -g3 -fsanitize=address,undefined -Wno-macro-redefined")

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics -stdlib=libc++ -Wall -Wextra -pedantic -O3 ${SEC_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3") #valgrind
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -g3 -fsanitize=address,undefined -Wno-macro-redefined")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SEC_LINK_FLAGS}")

include(deps/uv.cmake)
set(LIBUV_LIBRARIES uv)

set(SOURCE_FILES main.cc uvcast.h)

add_executable(sunrise ${SOURCE_FILES})
target_link_libraries(sunrise ${LIBUV_LIBRARIES})
target_link_libraries(sunrise pthread)
