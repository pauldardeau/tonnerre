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
# escapes a suite, or when no tests ran at all. Every test, the isolated
# receive-path scenarios included, runs in this one CTest entry. (Until the
# receive deadline existed, three scenarios were known to fail and ran in a
# second entry, tonnerre_known_defects, so that they could not hide, or be hidden
# by, the rest. They pass now and that entry is gone.)
add_test(
   NAME tonnerre_unit
   COMMAND test_tonnerre --group unit
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

set_tests_properties(tonnerre_unit PROPERTIES
   LABELS "unit"
   TIMEOUT 600
   # a zero-test run exits 2 and is reported as a failure; this also makes
   # CTest fail if the summary line is ever missing from the output
   FAIL_REGULAR_EXPRESSION "RESULT: FAIL"
)

# The production default receive deadline (Message::DEFAULT_RECEIVE_DEADLINE_MILLIS,
# 10 s) is checked end to end by one scenario that has to wait the whole 10 s out.
# It is skipped unless TONNERRE_TEST_SLOW is set, so tonnerre_unit (and the plain
# ./test_tonnerre run) never waits for it. This entry sets the variable and runs
# only that scenario, so the wait is paid exactly once, and carries the label
# "slow" so that it can be selected (ctest -L slow) or left out (ctest -LE slow).
# test_tonnerre exits 2 if no scenario with that id ran, so a renamed scenario or
# a missing variable fails this entry rather than passing it. The scenario's
# child process has a 20 s limit, so the 30 s CTest timeout is a second backstop.
# POSIX only, like the scenarios themselves.
if(NOT WIN32)
   add_test(
      NAME tonnerre_default_deadline
      COMMAND test_tonnerre --suite TestReceivePath
                            --scenario deadline.default-applies-to-reconstitute
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
   )

   set_tests_properties(tonnerre_default_deadline PROPERTIES
      LABELS "slow"
      TIMEOUT 30
      ENVIRONMENT "TONNERRE_TEST_SLOW=1"
      FAIL_REGULAR_EXPRESSION "RESULT: FAIL"
   )
endif()
