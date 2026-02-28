# Common library
add_library(common SHARED
    common.cpp
    common_api.cpp
    image_shared_memory.cpp
    common.h
    common_api.h
    common_global.h
    image_shared_memory.h
)

# Set target properties
target_compile_definitions(common PRIVATE COMMON_LIB)

# Link Qt libraries
target_link_libraries(common
    Qt6::Core
    Qt6::Gui
)

# Link optional dependencies if available
if(OpenCV_FOUND)
    target_link_libraries(common opencv_core opencv_imgproc)
endif()

if(spdlog_FOUND)
    target_link_libraries(common spdlog::spdlog)
endif()

# Set output name to match original
set_target_properties(common PROPERTIES
    OUTPUT_NAME common
    VERSION 1.0.0
    SOVERSION 1
)

# Include directories
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)