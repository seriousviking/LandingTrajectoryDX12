#CMake project
project(LandingTrajectoryDX12)
cmake_minimum_required(VERSION 3.1)

#project version
set(LandingTrajectoryDX12_VERSION_MAJOR 0)
set(LandingTrajectoryDX12_VERSION_MINOR 1)

#Release or debug
option(BUILD_RELEASE
	"On to build release version, off for debug"
	OFF
)

if (BUILD_RELEASE)
	message("Release build type")
	set( CMAKE_BUILD_TYPE Release )
else()
	message("Debug build type")
	set( CMAKE_BUILD_TYPE Debug )
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	message("Build x64")
	set (BIT_SUFFIX	"x64")
	set (LINKDIR_SUFFIX win64 )
else()
	message("Build x86")
	set (BIT_SUFFIX	"x86")
	set (LINKDIR_SUFFIX win32 )
	#set_target_properties(MyTarget PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

set(WINSDK_PATH "c:/Program Files (x86)/Windows Kits/10")
set(WINSDK_INCLUDE_PATH "${WINSDK_PATH}/Include/10.0.14393.0/um")
set(WINSDK_INCLUDE_SHARED_PATH "${WINSDK_PATH}/Include/10.0.14393.0/shared")
set(WINSDK_LIB_PATH "${WINSDK_PATH}/Lib/10.0.14393.0/um/${BIT_SUFFIX}")

message("WINSDK_LIB_PATH is ${WINSDK_LIB_PATH}")

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-${BIT_SUFFIX} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-release-${BIT_SUFFIX} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-debug-${BIT_SUFFIX} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-${BIT_SUFFIX} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-release-${BIT_SUFFIX} )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LandingTrajectoryDX12_SOURCE_DIR}/build-cmake-debug-${BIT_SUFFIX} )

if (MSVC)
	add_definitions( -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00 -DUNICODE /MP )
	add_definitions( -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS )
	if (BUILD_RELEASE)
		add_definitions( /MT )
	else()
		add_definitions( /MTd )
	endif()
else()
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} + "-std=c++11 -Wall")
endif()

#executable parameters
include_directories(${WINSDK_INCLUDE_PATH})
include_directories(${WINSDK_INCLUDE_SHARED_PATH})
link_directories(${WINSDK_LIB_PATH})

# sources
# main source files placed in root "src" folder

set (main_SRC
	src/main.cpp
	src/Application.h
	src/Application.cpp
	src/InputHandler.h
	src/InputHandler.cpp
	src/GraphicsManager.h
	src/GraphicsManager.cpp
	src/D3DManager.h
	src/D3DManager.cpp
	src/GraphicsCommon.h
)
source_group( src FILES ${main_SRC} )

# common sources
set (utils_SRC
	src/Utils/Types.h
	src/Utils/Macro.h
	src/Utils/Log.h
	src/Utils/Log.cpp
	src/Utils/FileUtils.h
	src/Utils/FileUtils.cpp
)
if (MSVC)
	source_group( src\\Utils FILES ${utils_SRC} )
else()
	source_group( src/Utils FILES ${utils_SRC} )
endif()

# add DirectXMath library

set (DIRECTXMATH_PATH "../Lib/DirectXMath")
set (DirectXMathInc
	${DIRECTXMATH_PATH}/Inc/DirectXCollision.h
	${DIRECTXMATH_PATH}/Inc/DirectXColors.h
	${DIRECTXMATH_PATH}/Inc/DirectXMath.h
	${DIRECTXMATH_PATH}/Inc/DirectXPackedVector.h
	${DIRECTXMATH_PATH}/Inc/DirectXCollision.inl
	${DIRECTXMATH_PATH}/Inc/DirectXMathConvert.inl
	${DIRECTXMATH_PATH}/Inc/DirectXMathMatrix.inl
	${DIRECTXMATH_PATH}/Inc/DirectXMathMisc.inl
	${DIRECTXMATH_PATH}/Inc/DirectXMathVector.inl
	${DIRECTXMATH_PATH}/Inc/DirectXPackedVector.inl
)
set (DirectXMathExtensions
	${DIRECTXMATH_PATH}/Extensions/DirectXMathAVX.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathAVX2.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathBE.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathF16C.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathFMA3.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathFMA4.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathSSE3.h
	${DIRECTXMATH_PATH}/Extensions/DirectXMathSSE4.h
)
set (DirectXMath
	${DirectXMathInc}
	${DirectXMathExtensions}
)
if (MSVC)
	source_group( Lib\\DirectXMath\\Inc FILES ${DirectXMathInc} )
	source_group( Lib\\DirectXMath\\Extensions FILES ${DirectXMathExtensions} )
else()
	source_group( Lib/DirectXMath/Inc FILES ${DirectXMathInc} )
	source_group( Lib/DirectXMath/Extensions FILES ${DirectXMathExtensions} )
endif()
include_directories(${DIRECTXMATH_PATH})

set (D3DHELPERS_PATH "../Lib/D3DHelpers")
set (D3DHelpers
	${D3DHELPERS_PATH}/d3dx12.h
)
if (MSVC)
	source_group( Lib\\D3DHelpers FILES ${D3DHelpers} )
else()
	source_group( Lib/D3DHelpers FILES ${D3DHelpers} )
endif()
include_directories(${D3DHELPERS_PATH})

# add shader files to group
set (ShadersPath "data/shaders")
set (VertexShader_Files
	${ShadersPath}/simpleVS.hlsl
)
set (PixelShader_Files
	${ShadersPath}/simplePS.hlsl
)
set (ShaderFiles
	${VertexShader_Files}
	${PixelShader_Files}
)
if (MSVC)
	source_group( data\\shaders FILES ${ShaderFiles} )
else()
	source_group( data/shaders FILES ${ShaderFiles} )
endif()

set_property(SOURCE ${VertexShader_Files} PROPERTY VS_SHADER_TYPE Vertex)
set_property(SOURCE ${VertexShader_Files} PROPERTY VS_SHADER_ENTRYPOINT mainVS)
set_property(SOURCE ${VertexShader_Files} PROPERTY VS_SHADER_MODEL 5.0)
# do not add any properties for now but keep them in mind
#set_property(SOURCE ${VertexShader_Files} PROPERTY VS_SHADER_FLAGS "/DFLAGS_ADDED /Fh \"$(OutDir)%(Filename).h\"")
set_property(SOURCE ${PixelShader_Files} PROPERTY VS_SHADER_TYPE Pixel)
set_property(SOURCE ${PixelShader_Files} PROPERTY VS_SHADER_ENTRYPOINT mainPS)
set_property(SOURCE ${PixelShader_Files} PROPERTY VS_SHADER_MODEL 5.0)
# do not add any properties for now but keep them in mind
#set_property(SOURCE ${PixelShader_Files} PROPERTY VS_SHADER_FLAGS "/DFLAGS_ADDED /Fh \"$(OutDir)%(Filename).h\"")

# executable and linked libraries
add_executable (LandingTrajectoryDX12 WIN32 ${main_SRC} ${utils_SRC} ${DirectXMath} ${D3DHelpers} ${ShaderFiles})
target_link_libraries (LandingTrajectoryDX12 d3d12 dxgi d3dcompiler)
