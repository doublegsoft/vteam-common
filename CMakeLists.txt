cmake_minimum_required(VERSION 3.13) 

project(vteam-common)

set(CMAKE_CXX_STANDARD 17)

if(APPLE) # Check if it's any Apple platform (macOS, iOS, etc.)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(PLATFORM "darwin")
  endif()
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
  set(PLATFORM "mingw64")
endif()


include_directories(
  "../vteam-3rd/cpp-httplib"
  "../vteam-3rd/base64/include"
  "../vteam-3rd/json-3.11.3/include"
  "../vteam-3rd/openssl-3.4.1/include"
  "../vteam-3rd/openssl-3.4.1/build/${PLATFORM}/include"
  "../vteam-3rd/inih-r58/cpp"
  "src"
)

link_directories(
  "../vteam-3rd/openssl-3.4.1/build/${PLATFORM}/"
)

add_executable(test_proc_ls 
  "test/test_proc_ls.cpp"
)

message(WARNING ${CMAKE_SYSTEM_NAME})
add_executable(test_http_post
  "test/test_http_post.cpp"
)
target_link_libraries(test_http_post  
  ssl 
  crypto 
  pthread
)
if(CMAKE_SYSTEM_NAME STREQUAL "Windows" OR CMAKE_SYSTEM_NAME STREQUAL "MSYS")
  target_link_libraries(test_http_post  
    ws2_32
    crypt32
  )
endif()

add_executable(test_ini_parse
  "test/test_ini_parse.cpp"
  "../vteam-3rd/inih-r58/cpp/INIReader.cpp"
  "../vteam-3rd/inih-r58/ini.c"
)