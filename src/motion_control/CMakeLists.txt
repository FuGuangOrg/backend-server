# Motion Control library
add_library(motion_control SHARED
    motion_control.cpp
    motion_control_plc.cpp
    motion_control_port.cpp
    motion_control.h
    motion_control_plc.h
    motion_control_port.h
    motion_control_global.h
)

# Link dependencies
target_link_libraries(motion_control
    Qt6::Core
    common
)

# Add libmodbus if available
if(LIBMODBUS_FOUND)
    target_link_libraries(motion_control ${LIBMODBUS_LIBRARIES})
    target_include_directories(motion_control PRIVATE ${LIBMODBUS_INCLUDE_DIRS})
    target_compile_options(motion_control PRIVATE ${LIBMODBUS_CFLAGS_OTHER})
endif()

# Set output name to match original
set_target_properties(motion_control PROPERTIES
    OUTPUT_NAME motion_control
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(motion_control PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)