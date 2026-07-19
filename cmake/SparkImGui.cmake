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
    add_library(spark_imgui STATIC
            "${_imgui_root}/imgui.cpp"
            "${_imgui_root}/imgui_draw.cpp"
            "${_imgui_root}/imgui_tables.cpp"
            "${_imgui_root}/imgui_widgets.cpp"
            "${_imgui_root}/imgui_demo.cpp"
            "${_imgui_root}/backends/imgui_impl_glfw.cpp"
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
