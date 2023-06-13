set(sonata-sources
    hdf5_lib.cpp
    data_management_lib.cpp
    dynamics_params_helper.cpp
    csv_lib.cpp
)

add_library(sonata ${sonata-sources})

add_library(sonata-public-headers INTERFACE)
add_library(sonata-private-headers INTERFACE)

target_include_directories(sonata-public-headers INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
target_include_directories(sonata-private-headers INTERFACE
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_include_directories(sonata PUBLIC ${HDF5_C_INCLUDE_DIRS})
target_link_libraries(sonata PUBLIC sonata-public-headers arbor::arbor arbor::arborio arbor::arborenv ${HDF5_C_LIBRARIES})
target_link_libraries(sonata PRIVATE sonata-private-deps arbor::arbor arbor::arborio arbor::arborenv ${HDF5_C_LIBRARIES})

install(DIRECTORY include/sonata
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")