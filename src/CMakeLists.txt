# Static by default (respects BUILD_SHARED_LIBS), same convention as
# poivre/chaudiere/misere. Doesn't affect the Makefile-built tonnerre.so.
add_library(tonnerre
   Message.cpp
   MessageRequestHandler.cpp
   MessageSocketServiceHandler.cpp
   Messaging.cpp
   MessagingServer.cpp
)

# The Makefile doesn't set an explicit -std for this directory (unlike
# poivre/chaudiere/misere), so it's been building against whatever the
# compiler's default standard is. cxx_std_20 here is a real behavior
# change versus that Makefile, not just a restatement of it - but a
# harmless one in practice, since it comes in transitively from
# chaudiere's own PUBLIC cxx_std_20 requirement regardless of whether
# it's stated here too (tonnerre already includes chaudiere headers
# that require it). Verified this compiles and passes tests under C++20.
target_compile_features(tonnerre PUBLIC cxx_std_20)
set_target_properties(tonnerre PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(tonnerre PRIVATE
   $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra>
)

# Headers live at this directory's root (no include/ subdirectory), so
# this is the one directory a consumer needs on its include path.
target_include_directories(tonnerre
   PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
      $<INSTALL_INTERFACE:include/tonnerre>
)

# PUBLIC: tonnerre's own public headers use chaudiere types, so a
# consumer of tonnerre also needs chaudiere's include directory and
# link library. Unlike chaudiere/misere, tonnerre's library itself has
# no poivre dependency at all (verified: no AutoPointer/poivre:: usage
# anywhere under src/), so there's no private poivre include needed here.
target_link_libraries(tonnerre PUBLIC chaudiere)

include(GNUInstallDirs)

install(TARGETS tonnerre
   EXPORT tonnerreTargets
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
   LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

file(GLOB TONNERRE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(FILES ${TONNERRE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tonnerre)
