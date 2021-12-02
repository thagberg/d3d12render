cmake_minimum_required(VERSION 3.17)

set(RenderSourceDir ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(RenderPublicIncludeDir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(RenderPrivateIncludeDir ${RenderPublicIncludeDir}/private)
set(RenderLibDir ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(RenderBinDir ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB RenderSources ${RenderSourceDir}/*.cpp)
file(GLOB RenderHeaders ${RenderPublicIncludeDir}/*.h)

add_library(libRender STATIC 
	${RenderSources} 
	${RenderHeaders}
	${RenderSourceDir}/Allocator/D3D12MemAlloc.cpp
	${RenderPrivateIncludeDir}/Allocator/D3D12MemAlloc.h)

set_property(TARGET libRender PROPERTY CXX_STANDARD 20)

target_include_directories(libRender PUBLIC ${RenderPublicIncludeDir})
target_include_directories(libRender PUBLIC ${RenderPrivateIncludeDir}/Allocator)
target_include_directories(libRender PRIVATE ${RenderPrivateIncludeDir})

target_link_directories(libRender PUBLIC ${RenderLibDir})
target_link_libraries(libRender PUBLIC d3d12)
target_link_libraries(libRender PUBLIC dxgi)
target_link_libraries(libRender PUBLIC dxcompiler)
target_link_libraries(libRender PUBLIC WinPixEventRuntime)

add_custom_target(DeployRenderBin
	COMMAND xcopy /E /I /Y \"${RenderBinDir}\" \"${CMAKE_BINARY_DIR}\")

add_dependencies(libRender DeployRenderBin)

set(TestDir ${RenderSourceDir}/sample)
file(GLOB TestShaders ${TestDir}/shaders/*.hlsl)
add_executable(RenderFramegraphTest WIN32 ${TestDir}/main.cpp ${TestShaders})
set_property(TARGET RenderFramegraphTest PROPERTY CXX_STANDARD 20)
set_property(TARGET RenderFramegraphTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
target_link_libraries(RenderFramegraphTest PUBLIC libRender)
set_source_files_properties(${TestShaders} PROPERTIES VS_TOOL_OVERRIDE "None")
source_group("TestShaders" FILES ${TestShaders})
add_custom_target(deploy_test_shaders ALL COMMAND xcopy /E /I /Y \"${TestDir}/shaders\" \"${CMAKE_BINARY_DIR}/testshaders\")
add_dependencies(RenderFramegraphTest deploy_test_shaders)
