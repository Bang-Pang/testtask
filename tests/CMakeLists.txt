find_package(GTest REQUIRED)

enable_testing()

add_executable(test_integrator test_integrator.cpp)
target_link_libraries(test_integrator PRIVATE common GTest::GTest GTest::Main)

add_test(NAME test_integrator COMMAND test_integrator)
