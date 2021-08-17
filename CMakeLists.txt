cmake_minimum_required(VERSION 3.17)

set(RenderSourceDir "src")
set(RenderPublicIncludeDir ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(RenderPrivateIncludeDir ${RenderPublicIncludeDir}/private)
set(RenderLibDir ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(RenderBinDir ${CMAKE_CURRENT_SOURCE_DIR}/bin)

file(GLOB RenderSources ${RenderSourceDir}/*.cpp)
file(GLOB RenderHeaders ${RenderPublicIncludeDir}/*.h)
add_library(libRender STATIC ${RenderSources} ${RenderHeaders})
set_propertY(TARGET libRender PROPERTY CXX_STANDARD 20)
target_include_directories(libRender PUBLIC ${RenderPublicIncludeDir})
target_include_directories(libRender PRIVATE ${RenderPrivateIncludeDir})

target_link_directories(libRender PUBLIC ${RenderLibDir})
target_link_libraries(libRender PUBLIC d3d12)
target_link_libraries(libRender PUBLIC dxgi)
target_link_libraries(libRender PUBLIC dxcompiler)

add_custom_target(DeployRenderBin
	COMMAND xcopy /E /I /Y \"${RenderBinDir}\" \"${CMAKE_BINARY_DIR}\")

add_dependencies(libRender DeployRenderBin)
