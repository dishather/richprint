cmake_minimum_required( VERSION 2.8 )
project( RichPrint )
set( CMAKE_BUILD_TYPE Release )
string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}"  )
message(STATUS "${CMAKE_CXX_FLAGS_RELEASE} " CMAKE_CXX_FLAGS_RELEASE)
add_executable( richprint richprint.cpp )

install(TARGETS richprint DESTINATION bin)

INCLUDE(CPack)
