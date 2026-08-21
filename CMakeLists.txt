cmake_minimum_required(VERSION 3.16)

project(tonnerre
   DESCRIPTION "Simple messaging between applications over TCP/IP"
   LANGUAGES CXX
)

# True only when tonnerre is configured directly, not pulled in via
# add_subdirectory() by a consumer. Controls whether tonnerre's own
# test suite is built by default.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
   set(TONNERRE_IS_TOP_LEVEL TRUE)
else()
   set(TONNERRE_IS_TOP_LEVEL FALSE)
endif()

option(TONNERRE_BUILD_TESTS "Build tonnerre's own test suite" ${TONNERRE_IS_TOP_LEVEL})

# chaudiere's own CHAUDIERE_BUILD_TESTS defaults off here (same
# CMAKE_SOURCE_DIR check, and this isn't chaudiere's own top-level
# configure), so this doesn't pull in chaudiere's tests or poivre.
add_subdirectory(chaudiere)
add_subdirectory(src)

if(TONNERRE_BUILD_TESTS)
   add_subdirectory(poivre)
   enable_testing()
   add_subdirectory(test)
endif()
