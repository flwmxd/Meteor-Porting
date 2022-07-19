cmake_minimum_required(VERSION 3.4.1)

project(MapleAssembly VERSION 0.1.0 LANGUAGES CSharp)
INCLUDE(CSharpUtilities)

get_filename_component(LIB_SRC_DIR
                       ${CMAKE_CURRENT_LIST_DIR}
                       ABSOLUTE)

message( ${CMAKE_CURRENT_LIST_DIR})



file(GLOB GAME_APP_SRC
	${LIB_SRC_DIR}/*.cs
	${LIB_SRC_DIR}/scripts/*.cs
)

if (${Target} MATCHES "Windows")

#set(LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/Scripts)

SET(CMAKE_CSharp_FLAGS "/platform:AnyCPU /unsafe")


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "MapleAssembly")

source_group(TREE ${LIB_SRC_DIR} FILES ${GAME_APP_SRC})

add_library(MapleAssembly SHARED ${GAME_APP_SRC})
set_property(TARGET MapleAssembly PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v4.6.1")
set_property(
	TARGET 
	MapleAssembly 
	PROPERTY 
	VS_DOTNET_REFERENCES
	"System;System.Core;System.Numerics;System.Runtime;System.Runtime.CompilerServices.Unsafe"
)

target_link_libraries(MapleAssembly MapleLibrary)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

endif()


