# Dear ImGui (docking branch) + GLFW/Vulkan backends for Spark tool UI.
function(spark_setup_imgui target_name)
    include(FetchContent)
    FetchContent_Declare(
            imgui
            GIT_REPOSITORY https://github.com/ocornut/imgui.git
            GIT_TAG docking
    )
    FetchContent_MakeAvailable(imgui)

    set(_imgui_root "${imgui_SOURCE_DIR}")
    set(_imgui_glfw_backend_src "${CMAKE_BINARY_DIR}/spark_imgui/imgui_impl_glfw_patched.cpp")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/spark_imgui")
    file(READ "${_imgui_root}/backends/imgui_impl_glfw.cpp" _spark_imgui_glfw_src)
    string(REPLACE
            "        GLFWwindow* window = (GLFWwindow*)viewport->PlatformHandle;\n\n#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3"
            "        GLFWwindow* window = (GLFWwindow*)viewport->PlatformHandle;\n        if (window == nullptr)\n            continue;\n\n#ifdef EMSCRIPTEN_USE_EMBEDDED_GLFW3"
            _spark_imgui_glfw_src "${_spark_imgui_glfw_src}")
    string(REPLACE
            "        GLFWwindow* window = (GLFWwindow*)platform_io.Viewports[n]->PlatformHandle;\n        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)"
            "        GLFWwindow* window = (GLFWwindow*)platform_io.Viewports[n]->PlatformHandle;\n        if (window == nullptr)\n            continue;\n        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)"
            _spark_imgui_glfw_src "${_spark_imgui_glfw_src}")
    file(WRITE "${_imgui_glfw_backend_src}" "${_spark_imgui_glfw_src}")

    add_library(spark_imgui STATIC
            "${_imgui_root}/imgui.cpp"
            "${_imgui_root}/imgui_draw.cpp"
            "${_imgui_root}/imgui_tables.cpp"
            "${_imgui_root}/imgui_widgets.cpp"
            "${_imgui_root}/imgui_demo.cpp"
            "${_imgui_glfw_backend_src}"
            "${_imgui_root}/backends/imgui_impl_vulkan.cpp"
    )
    target_include_directories(spark_imgui PUBLIC
            "${_imgui_root}"
            "${_imgui_root}/backends"
    )
    target_link_libraries(spark_imgui PUBLIC glfw Vulkan::Vulkan)
    target_compile_definitions(spark_imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS=1)

    target_compile_definitions(${target_name} PUBLIC SPARK_ENABLE_IMGUI=1)
    target_sources(${target_name} PRIVATE
            src/spark/imgui/ImGuiVulkanLayer.cpp
            src/spark/imgui/ImGuiLayerFactory.cpp
    )
    target_link_libraries(${target_name} PRIVATE spark_imgui)
endfunction()
