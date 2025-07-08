cmake_minimum_required(VERSION 3.21)
project(DirectX11Engine)

set(CMAKE_CXX_STANDARD 17)

# Set the source directory
set(SRC_DIR ${CMAKE_SOURCE_DIR}/Engine/src)
set(ASSETS_DIR ${CMAKE_SOURCE_DIR}/Engine/assets)

# Set FBX SDK and Qt local paths
set(FBX_SDK_ROOT "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/FBX SDK/2020.3.7")
set(QT_ROOT "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/Qt/6.8.3/msvc2022_64")

# Set library directories for each configuration
set(FBXSDK_LIB_RELEASE "${FBX_SDK_ROOT}/lib/x64/release")
set(FBXSDK_LIB_DEBUG   "${FBX_SDK_ROOT}/lib/x64/debug")

# Make CMake look for Qt in the ThirdParty folder
set(CMAKE_PREFIX_PATH "${QT_ROOT}")
set(Qt6_DIR "${QT_ROOT}/lib/cmake/Qt6")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Verify Qt was found
if(TARGET Qt6::Core)
    message(STATUS "Qt6 Core found")
else()
    message(FATAL_ERROR "Qt6 Core not found!")
endif()

if(TARGET Qt6::Widgets)
    message(STATUS "Qt6 Widgets found")
else()
    message(FATAL_ERROR "Qt6 Widgets not found!")
endif()

# Enable Qt's automatic features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    ${SRC_DIR}/Entry/*.cpp
    ${SRC_DIR}/Core/**/*.cpp
    ${SRC_DIR}/Graphics/**/*.cpp
    ${SRC_DIR}/Math/**/*.cpp
    ${SRC_DIR}/GUI/**/*.cpp
)

file(GLOB_RECURSE ENGINE_HEADERS
    ${SRC_DIR}/Entry/*.h
    ${SRC_DIR}/Core/**/*.h
    ${SRC_DIR}/Graphics/**/*.h
    ${SRC_DIR}/Math/**/*.h
    ${SRC_DIR}/GUI/**/*.h
)

# Gather shader files
file(GLOB_RECURSE SHADER_FILES
    ${ASSETS_DIR}/shaders/*.hlsl
)

# Add the executable
add_executable(${PROJECT_NAME}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}/Entry
    ${SRC_DIR}/Core
    ${SRC_DIR}/Graphics
    ${SRC_DIR}/Math
    ${SRC_DIR}/GUI
    "${FBX_SDK_ROOT}/include"
)

# Link directories for FBX SDK (per configuration)
target_link_directories(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:${FBXSDK_LIB_DEBUG}>
    $<$<CONFIG:Release>:${FBXSDK_LIB_RELEASE}>
)

# Link Qt and FBX SDK libraries (per configuration)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core 
    Qt6::Widgets 
    $<$<CONFIG:Debug>:libfbxsdk-md>
    $<$<CONFIG:Release>:libfbxsdk-md>
    $<$<CONFIG:Debug>:libxml2-md>
    $<$<CONFIG:Release>:libxml2-md>
    $<$<CONFIG:Debug>:zlib-md>
    $<$<CONFIG:Release>:zlib-md>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN     # Reduces Windows header bloat
    NOMINMAX                # Prevents min/max macro definitions
    UNICODE                 # Use Unicode
    _UNICODE                # Use Unicode
)

# (Optional) Set the output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    AUTOMOC_MOC_OPTIONS "-DNOMINMAX;-DWIN32_LEAN_AND_MEAN;-DQT_NO_KEYWORDS"
)

# Add shader files to the project
target_sources(${PROJECT_NAME} PRIVATE ${SHADER_FILES})
set_source_files_properties(${SHADER_FILES} PROPERTIES
    HEADER_FILE_ONLY TRUE
    VS_TOOL_OVERRIDE "None"
)

# Create source groups for shader files
source_group("assets\\shaders" FILES ${SHADER_FILES})

# Post-build: Copy shader files to output directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_DIR}/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)

# Post-build: Copy required DLLs from FBX SDK and Qt to output bin directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<$<CONFIG:Debug>:${QT_ROOT}/plugins/platforms/qwindowsd.dll>$<$<CONFIG:Release>:${QT_ROOT}/plugins/platforms/qwindows.dll>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/platforms/qwindowsd.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FBX_SDK_ROOT}/bin/libfbxsdk.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6Cored.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6Guid.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6OpenGLd.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6Widgetsd.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6OpenGLWidgetsd.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Entry
source_group("src\\Entry" FILES
    ${SRC_DIR}/Entry/main.cpp
)

# Core
source_group("src\\Core\\Application" FILES
    ${SRC_DIR}/Core/Application/Application.cpp
    ${SRC_DIR}/Core/Application/Application.h
)
source_group("src\\Core\\Common" FILES
    ${SRC_DIR}/Core/Common/EngineTypes.h
)
source_group("src\\Core\\Input\\Management" FILES
    ${SRC_DIR}/Core/Input/Management/InputManager.cpp
    ${SRC_DIR}/Core/Input/Management/InputManager.h
)
source_group("src\\Core\\System" FILES
    ${SRC_DIR}/Core/System/SystemManager.cpp
    ${SRC_DIR}/Core/System/SystemManager.h
    ${SRC_DIR}/Core/System/Timer.cpp
    ${SRC_DIR}/Core/System/Timer.h
    ${SRC_DIR}/Core/System/Logger.h
    ${SRC_DIR}/Core/System/PerformanceProfiler.cpp
    ${SRC_DIR}/Core/System/PerformanceProfiler.h
    ${SRC_DIR}/Core/System/CommonTimer.cpp
    ${SRC_DIR}/Core/System/CommonTimer.h
    ${SRC_DIR}/Core/System/PerformanceLogger.cpp
    ${SRC_DIR}/Core/System/PerformanceLogger.h
)

# Graphics
source_group("src\\Graphics\\D3D11" FILES
    ${SRC_DIR}/Graphics/D3D11/D3D11Device.cpp
    ${SRC_DIR}/Graphics/D3D11/D3D11Device.h
)
source_group("src\\Graphics\\Math" FILES
    ${SRC_DIR}/Graphics/Math/Frustum.cpp
    ${SRC_DIR}/Graphics/Math/Frustum.h
    ${SRC_DIR}/Graphics/Math/Position.cpp
    ${SRC_DIR}/Graphics/Math/Position.h
)
source_group("src\\Graphics\\Rendering" FILES
    ${SRC_DIR}/Graphics/Rendering/Camera.cpp
    ${SRC_DIR}/Graphics/Rendering/Camera.h
    ${SRC_DIR}/Graphics/Rendering/DisplayPlane.cpp
    ${SRC_DIR}/Graphics/Rendering/DisplayPlane.h
    ${SRC_DIR}/Graphics/Rendering/Font.cpp
    ${SRC_DIR}/Graphics/Rendering/Font.h
    ${SRC_DIR}/Graphics/Rendering/Light.cpp
    ${SRC_DIR}/Graphics/Rendering/Light.h
    ${SRC_DIR}/Graphics/Rendering/RenderTexture.cpp
    ${SRC_DIR}/Graphics/Rendering/RenderTexture.h
    ${SRC_DIR}/Graphics/Rendering/Sprite.cpp
    ${SRC_DIR}/Graphics/Rendering/Sprite.h
)
source_group("src\\Graphics\\Rendering\\Utils" FILES
    ${SRC_DIR}/Graphics/Rendering/Utils/RenderUtils.cpp
    ${SRC_DIR}/Graphics/Rendering/Utils/RenderUtils.h
)
source_group("src\\Graphics\\Resource" FILES
    ${SRC_DIR}/Graphics/Resource/Model.cpp
    ${SRC_DIR}/Graphics/Resource/Model.h
    ${SRC_DIR}/Graphics/Resource/Text.cpp
    ${SRC_DIR}/Graphics/Resource/Text.h
    ${SRC_DIR}/Graphics/Resource/Texture.cpp
    ${SRC_DIR}/Graphics/Resource/Texture.h
)
source_group("src\\Graphics\\Resource\\Environment" FILES
    ${SRC_DIR}/Graphics/Resource/Environment/Skybox.cpp
    ${SRC_DIR}/Graphics/Resource/Environment/Skybox.h
    ${SRC_DIR}/Graphics/Resource/Environment/SkyDome.cpp
    ${SRC_DIR}/Graphics/Resource/Environment/SkyDome.h
    ${SRC_DIR}/Graphics/Resource/Environment/Zone.cpp
    ${SRC_DIR}/Graphics/Resource/Environment/Zone.h
)
source_group("src\\Graphics\\Scene\\Management" FILES
    ${SRC_DIR}/Graphics/Scene/Management/ModelList.cpp
    ${SRC_DIR}/Graphics/Scene/Management/ModelList.h
    ${SRC_DIR}/Graphics/Scene/Management/SelectionManager.cpp
    ${SRC_DIR}/Graphics/Scene/Management/SelectionManager.h
)
source_group("src\\Graphics\\Shaders" FILES
    ${SRC_DIR}/Graphics/Shaders/AlphaMapShader.cpp
    ${SRC_DIR}/Graphics/Shaders/AlphaMapShader.h
    ${SRC_DIR}/Graphics/Shaders/ColorShader.cpp
    ${SRC_DIR}/Graphics/Shaders/ColorShader.h
    ${SRC_DIR}/Graphics/Shaders/FontShader.cpp
    ${SRC_DIR}/Graphics/Shaders/FontShader.h
    ${SRC_DIR}/Graphics/Shaders/LightMapShader.cpp
    ${SRC_DIR}/Graphics/Shaders/LightMapShader.h
    ${SRC_DIR}/Graphics/Shaders/LightShader.cpp
    ${SRC_DIR}/Graphics/Shaders/LightShader.h
    ${SRC_DIR}/Graphics/Shaders/MultiTextureShader.cpp
    ${SRC_DIR}/Graphics/Shaders/MultiTextureShader.h
    ${SRC_DIR}/Graphics/Shaders/NormalMapShader.cpp
    ${SRC_DIR}/Graphics/Shaders/NormalMapShader.h
    ${SRC_DIR}/Graphics/Shaders/SpecularMapShader.cpp
    ${SRC_DIR}/Graphics/Shaders/SpecularMapShader.h
    ${SRC_DIR}/Graphics/Shaders/TextureShader.cpp
    ${SRC_DIR}/Graphics/Shaders/TextureShader.h
    ${SRC_DIR}/Graphics/Shaders/PBRShader.h
    ${SRC_DIR}/Graphics/Shaders/PBRShader.cpp
    ${SRC_DIR}/Graphics/Shaders/BaseShader.h
    ${SRC_DIR}/Graphics/Shaders/BaseShader.cpp
)
source_group("src\\Graphics\\Shaders\\Management" FILES
    ${SRC_DIR}/Graphics/Shaders/Management/ShaderManager.cpp
    ${SRC_DIR}/Graphics/Shaders/Management/ShaderManager.h
)
source_group("src\\Graphics\\Shaders\\Environment" FILES
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyboxShader.cpp
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyboxShader.h
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyDomeShader.cpp
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyDomeShader.h
)
source_group("src\\Graphics\\Utils" FILES
    ${SRC_DIR}/Graphics/Utils/RenderUtils.cpp
    ${SRC_DIR}/Graphics/Utils/RenderUtils.h
)

# Math
source_group("src\\Math" FILES
    ${SRC_DIR}/Math/Transform.h
    ${SRC_DIR}/Math/Transform.cpp
)

# GUI
source_group("src\\GUI\\Windows" FILES
    ${SRC_DIR}/GUI/Windows/MainWindow.cpp
    ${SRC_DIR}/GUI/Windows/MainWindow.h
    ${SRC_DIR}/GUI/Windows/DirectXViewport.cpp
    ${SRC_DIR}/GUI/Windows/DirectXViewport.h
    ${SRC_DIR}/GUI/Windows/PerformanceWidget.cpp
    ${SRC_DIR}/GUI/Windows/PerformanceWidget.h
)
source_group("src\\GUI\\Components" FILES
    ${SRC_DIR}/GUI/Components/UserInterface.cpp
    ${SRC_DIR}/GUI/Components/UserInterface.h
    ${SRC_DIR}/GUI/Components/TransformUI.cpp
    ${SRC_DIR}/GUI/Components/TransformUI.h
    ${SRC_DIR}/GUI/Components/ModelListUI.cpp
    ${SRC_DIR}/GUI/Components/ModelListUI.h
)