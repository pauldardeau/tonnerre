# TestClient/TestServer are small interactive demo/exercise tools (run
# one against the other manually), not automated tests - matching the
# Makefile, they're built but not registered with ctest.
add_executable(TestClient TestClient.cpp)
target_link_libraries(TestClient PRIVATE tonnerre)

add_executable(TestServer TestServer.cpp)
target_link_libraries(TestServer PRIVATE tonnerre)

add_executable(test_tonnerre
   Tests.cpp
   TestMessaging.cpp
   TestMessagingServer.cpp
   TestMessage.cpp
   TestMessageRequestHandler.cpp
   TestMessageSocketServiceHandler.cpp
)

# The receive-path regression suite runs each scenario in a forked child
# process against an ephemeral-port loopback connection, so it is POSIX only.
if(NOT WIN32)
   target_sources(test_tonnerre PRIVATE
      IsolatedCase.cpp
      TestReceivePath.cpp
   )
endif()

# chaudiere comes in transitively via tonnerre's own PUBLIC link to it.
target_link_libraries(test_tonnerre PRIVATE tonnerre poivre)

# test_tonnerre exits nonzero when any assertion fails, when an exception
# escapes a suite, or when no tests ran at all. It is registered twice so a
# known-defect failure is visible on its own and cannot hide, or be hidden by,
# the established suites:
#   tonnerre_unit           established suites + scenarios with no known defect
#   tonnerre_known_defects  scenarios annotated with a known defect (these are
#                           expected to fail until the defect is fixed)
# `test_tonnerre` with no arguments runs both groups.
add_test(
   NAME tonnerre_unit
   COMMAND test_tonnerre --group unit
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT WIN32)
   add_test(
      NAME tonnerre_known_defects
      COMMAND test_tonnerre --group known-defects
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
   )
   set_tests_properties(tonnerre_known_defects PROPERTIES
      LABELS "known-defects"
      TIMEOUT 600
   )
endif()

set_tests_properties(tonnerre_unit PROPERTIES
   LABELS "unit"
   TIMEOUT 600
   # a zero-test run exits 2 and is reported as a failure; this also makes
   # CTest fail if the summary line is ever missing from the output
   FAIL_REGULAR_EXPRESSION "RESULT: FAIL"
)
