add_executable(arbata arbata.cpp)
target_link_libraries(arbata PRIVATE sonata arbor::arbor arbor::arborenv ${HDF5_C_LIBRARIES})