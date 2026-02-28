# Device Camera library
add_library(device_camera SHARED
    interface_camera.cpp
    camera_mvs.cpp
    camera_dvp2.cpp
    camera_factory.cpp
    grab_worker.cpp
    device_camera_global.h
    interface_camera.h
    grab_worker.h
    camera_mvs.h
    camera_dvp2.h
    camera_factory.h
)

# Link dependencies
target_link_libraries(device_camera
    Qt6::Core
    Qt6::Gui
    common
    device_enum
)

# Set output name to match original
set_target_properties(device_camera PROPERTIES
    OUTPUT_NAME device_camera
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(device_camera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)