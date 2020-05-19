cmake_minimum_required ( VERSION 3.15 )
project ( krabby )

option ( ENABLE_LOG   "Enables or disables logging support"  OFF )

MESSAGE ( STATUS "Krabby Options:" )
MESSAGE ( STATUS "----" )
MESSAGE ( STATUS "ENABLE_LOG:   " ${ENABLE_LOG} )
MESSAGE ( STATUS "----" )

set(CMAKE_CXX_STANDARD 17) # this is for crablib to work

add_subdirectory ( ${CMAKE_CURRENT_SOURCE_DIR}/lib/fmtlib EXCLUDE_FROM_ALL )
add_subdirectory ( ${CMAKE_CURRENT_SOURCE_DIR}/lib/inja EXCLUDE_FROM_ALL )
add_subdirectory ( ${CMAKE_CURRENT_SOURCE_DIR}/lib/crablib )
add_subdirectory ( ${CMAKE_CURRENT_SOURCE_DIR}/lib/sqlcpp_bridge )
add_subdirectory ( ${CMAKE_CURRENT_SOURCE_DIR}/lib/sol2 )

find_package(Lua)

file( GLOB_RECURSE ALL_SRC src/**.cpp src/**.hpp )
add_executable ( ${PROJECT_NAME}  "${ALL_SRC}" )

target_compile_features ( ${PROJECT_NAME} 
    PRIVATE  cxx_std_17 )

target_compile_options( ${PROJECT_NAME} 
    PRIVATE  "-Wno-logical-op-parentheses" ) # silences a sol2 warning

target_include_directories ( 
  ${PROJECT_NAME}
    PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/lib/cxxopts/include"
    PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE  "${LUA_INCLUDE_DIR}" )

target_link_libraries (
  ${PROJECT_NAME} 
    PRIVATE  "-lstdc++"
    PRIVATE  "${LUA_LIBRARIES}"
    PRIVATE  crablib::crablib
    PRIVATE  lib::SQLCppBridge
    PRIVATE  pantor::inja 
    PRIVATE  fmt::fmt-header-only
    PRIVATE  sol2::sol2 )

if ( ENABLE_LOG )
  target_compile_definitions (
    ${PROJECT_NAME}   PUBLIC    "ENABLE_LOG" )
endif()
