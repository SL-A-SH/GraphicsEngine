cmake_minimum_required(VERSION 3.21)
project(DirectX11Engine)

set(CMAKE_CXX_STANDARD 17)

# Set the source directory
set(SRC_DIR ${CMAKE_SOURCE_DIR}/Engine/src)

# Set FBX SDK and Qt local paths
set(FBX_SDK_ROOT "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/FBX SDK/2020.3.7")
set(QT_ROOT "${CMAKE_SOURCE_DIR}/Engine/ThirdParty/Qt/6.9.0/msvc2022_64")

# Set library directories for each configuration
set(FBXSDK_LIB_RELEASE "${FBX_SDK_ROOT}/lib/x64/release")
set(FBXSDK_LIB_DEBUG   "${FBX_SDK_ROOT}/lib/x64/debug")

# Make CMake look for Qt in the ThirdParty folder
set(CMAKE_PREFIX_PATH "${QT_ROOT}")

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable Qt's automatic features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    ${SRC_DIR}/Main/*.cpp
    ${SRC_DIR}/Core/**/*.cpp
    ${SRC_DIR}/Graphics/**/*.cpp
    ${SRC_DIR}/Math/**/*.cpp
    ${SRC_DIR}/GUI/**/*.cpp
)

file(GLOB_RECURSE ENGINE_HEADERS
    ${SRC_DIR}/Main/*.h
    ${SRC_DIR}/Core/**/*.h
    ${SRC_DIR}/Graphics/**/*.h
    ${SRC_DIR}/Math/**/*.h
    ${SRC_DIR}/GUI/**/*.h
)

# Add the executable
add_executable(${PROJECT_NAME}
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

# Include directories
# Qt includes are handled by find_package, but you can add them explicitly if needed
# target_include_directories(${PROJECT_NAME} PRIVATE "${QT_ROOT}/include")
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SRC_DIR}/Main
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
    Qt6::Core Qt6::Gui Qt6::Widgets
    $<$<CONFIG:Debug>:libfbxsdk-md>
    $<$<CONFIG:Release>:libfbxsdk-md>
    $<$<CONFIG:Debug>:libxml2-md>
    $<$<CONFIG:Release>:libxml2-md>
    $<$<CONFIG:Debug>:zlib-md>
    $<$<CONFIG:Release>:zlib-md>
)

# (Optional) Set the output directory for the executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Post-build: Copy required DLLs from FBX SDK and Qt to output bin directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FBX_SDK_ROOT}/bin/libfbxsdk.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${QT_ROOT}/bin/Qt6Core.dll" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
)

# Main
source_group("src\\Main" FILES
    ${SRC_DIR}/Main/main.cpp
)

# Core
source_group("src\\Core\\Application" FILES
    ${SRC_DIR}/Core/Application/Application.cpp
    ${SRC_DIR}/Core/Application/Application.h
)
source_group("src\\Core\\Input" FILES
    ${SRC_DIR}/Core/Input/InputManager.cpp
    ${SRC_DIR}/Core/Input/InputManager.h
)
source_group("src\\Core\\System" FILES
    ${SRC_DIR}/Core/System/SystemManager.cpp
    ${SRC_DIR}/Core/System/SystemManager.h
    ${SRC_DIR}/Core/System/Timer.cpp
    ${SRC_DIR}/Core/System/Timer.h
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
source_group("src\\Graphics\\Scene" FILES
    ${SRC_DIR}/Graphics/Scene/ModelList.cpp
    ${SRC_DIR}/Graphics/Scene/ModelList.h
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
    ${SRC_DIR}/Graphics/Shaders/ShaderManager.cpp
    ${SRC_DIR}/Graphics/Shaders/ShaderManager.h
    ${SRC_DIR}/Graphics/Shaders/SpecularMapShader.cpp
    ${SRC_DIR}/Graphics/Shaders/SpecularMapShader.h
    ${SRC_DIR}/Graphics/Shaders/TextureShader.cpp
    ${SRC_DIR}/Graphics/Shaders/TextureShader.h
)
source_group("src\\Graphics\\Shaders\\Environment" FILES
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyboxShader.cpp
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyboxShader.h
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyDomeShader.cpp
    ${SRC_DIR}/Graphics/Shaders/Environment/SkyDomeShader.h
)
source_group("src\\Graphics\\UI" FILES
    ${SRC_DIR}/Graphics/UI/UserInterface.cpp
    ${SRC_DIR}/Graphics/UI/UserInterface.h
)

# Math
source_group("src\\Math" FILES
    ${SRC_DIR}/Math/Transform.h
)

# GUI
source_group("src\\GUI\\Windows" FILES
    ${SRC_DIR}/GUI/Windows/MainWindow.cpp
    ${SRC_DIR}/GUI/Windows/MainWindow.h
)