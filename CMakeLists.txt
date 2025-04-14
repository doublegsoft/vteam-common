#              mm                                      
#              MM                                      
# `7M'   `MF'mmMMmm .gP"Ya   ,6"Yb.  `7MMpMMMb.pMMMb.  
#   VA   ,V    MM  ,M'   Yb 8)   MM    MM    MM    MM  
#    VA ,V     MM  8M""""""  ,pm9MM    MM    MM    MM  
#     VVV      MM  YM.    , 8M   MM    MM    MM    MM  
#      W       `Mbmo`Mbmmd' `Moo9^Yo..JMML  JMML  JMML.
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
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(PLATFORM "msvc")
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

add_executable(test_http_post
  "test/test_http_post.cpp"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_link_libraries(test_http_post  
    libssl 
    libcrypto 
  )
else()
  target_link_libraries(test_http_post  
    ssl 
    crypto 
    pthread
  )
endif()
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