# Auto Focus library
add_library(auto_focus SHARED
    auto_focus2.cpp
    thread_calc_image_clarity.cpp
    auto_focus_global.h
    auto_focus2.h
    thread_calc_image_clarity.h
)

# Link dependencies
target_link_libraries(auto_focus
    Qt6::Core
    Qt6::Gui
    common
    device_camera
)

# Set output name to match original
set_target_properties(auto_focus PROPERTIES
    OUTPUT_NAME auto_focus
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(auto_focus PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)