# Fiber End Server - Main Application
add_executable(fiber_end_server
    main.cpp
    server.cpp
    thread_algorithm.cpp
    thread_device_enum.cpp
    thread_misc.cpp
    thread_motion_control.cpp
    work_threads.cpp
    server.h
    thread_algorithm.h
    thread_device_enum.h
    thread_misc.h
    thread_motion_control.h
    work_threads.h
    camera_config_mgr.hpp
    config.hpp
    device_manager.hpp
)

# Link all dependencies
target_link_libraries(fiber_end_server
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Xml
    common
    device_enum
    device_camera
    motion_control
    auto_focus
    event_bus
)

# Link optional dependencies if available
if(OpenCV_FOUND)
    target_link_libraries(fiber_end_server opencv_core opencv_imgproc)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(fiber_end_server nlohmann_json::nlohmann_json)
endif()

if(asio_FOUND)
    target_link_libraries(fiber_end_server asio::asio)
endif()

if(pugixml_FOUND)
    target_link_libraries(fiber_end_server pugixml::static)
endif()

# Add CLI11 if available
if(CLI11_FOUND)
    target_link_libraries(fiber_end_server CLI11::CLI11)
elseif(CLI11_INCLUDE_DIRS)
    target_include_directories(fiber_end_server PRIVATE ${CLI11_INCLUDE_DIRS})
endif()

# Add inference capabilities if enabled
if(FUGUANG_ENABLE_INFERENCE)
    target_compile_definitions(fiber_end_server PRIVATE FUGUANG_ENABLE_INFERENCE)
    
    # Try to find onnxruntime
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
        PATHS ${CMAKE_SOURCE_DIR}/include/onnxruntime
        NO_DEFAULT_PATH
    )
    
    find_library(ONNXRUNTIME_LIBRARY 
        NAMES onnxruntime
        PATHS ${CMAKE_SOURCE_DIR}/lib/Debug ${CMAKE_SOURCE_DIR}/lib/Release
        NO_DEFAULT_PATH
    )
    
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
        target_include_directories(fiber_end_server PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
        target_link_libraries(fiber_end_server ${ONNXRUNTIME_LIBRARY})
        
        # Link basic_algorithm if inspection-algo is available
        if(TARGET basic_algorithm)
            target_link_libraries(fiber_end_server basic_algorithm)
        endif()
        
        message(STATUS "ONNX Runtime found and enabled for inference")
    else()
        message(WARNING "FUGUANG_ENABLE_INFERENCE is ON but ONNX Runtime not found")
    endif()
endif()

# Set output name to match original
set_target_properties(fiber_end_server PROPERTIES
    OUTPUT_NAME fiber_end_server
)

# Include directories
target_include_directories(fiber_end_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)