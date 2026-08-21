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

# chaudiere comes in transitively via tonnerre's own PUBLIC link to it.
target_link_libraries(test_tonnerre PRIVATE tonnerre poivre)

add_test(
   NAME test_tonnerre
   COMMAND test_tonnerre
   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
