# Tests CMakeLists.txt

# Fetch Catch2 testing framework
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

# Helper to add tests
function(add_luup_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME} PRIVATE luup_agent Catch2::Catch2WithMain)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Unit tests
add_luup_test(test_model unit/test_model.cpp)
add_luup_test(test_agent unit/test_agent.cpp)
add_luup_test(test_tools unit/test_tools.cpp)

# Integration tests (will add later)
# add_luup_test(test_local_inference integration/test_local_inference.cpp)
# add_luup_test(test_remote_api integration/test_remote_api.cpp)

