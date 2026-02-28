# Device Enum library
add_library(device_enum SHARED
    device_enum_factory.cpp
    device_enum_dvp2.cpp
    device_enum_mvs.cpp
    device_enum.h
    device_enum_factory.h
    device_enum_global.h
    device_enum_dvp2.h
    device_enum_mvs.h
)

# Link Qt libraries
target_link_libraries(device_enum
    Qt6::Core
)

# Set output name to match original
set_target_properties(device_enum PROPERTIES
    OUTPUT_NAME device_enum
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(device_enum PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)