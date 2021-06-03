add_executable(example example.cpp)
target_link_libraries(example PRIVATE sonata arbor::arbor arbor::arborenv ${HDF5_C_LIBRARIES})