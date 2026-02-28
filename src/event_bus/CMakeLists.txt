# Event Bus library
add_library(event_bus SHARED
    event_bus.cpp
    event_bus.h
    event_bus_global.h
    interface_event_bus.h
    event_register.h
)

# Link Qt libraries
target_link_libraries(event_bus
    Qt6::Core
)

# Set output name to match original
set_target_properties(event_bus PROPERTIES
    OUTPUT_NAME event_bus
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(event_bus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)