cmake_minimum_required(VERSION 3.4.1)

option(ENGINE_AS_LIBRARY "build engine as dynamic library" ON)
option(MAPLE_OPENGL "Opengl as the default renderer" ON)
option(MAPLE_VULKAN "Vulkan as the default renderer" OFF)

if(MAPLE_OPENGL)
	project(Meteor-Mobile-OpenGL)
endif()

if(MAPLE_VULKAN)
	project(Meteor-Mobile-Vulkan)
endif()

get_filename_component(LIB_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/MapleEngine/Code/Maple/lib
                       ABSOLUTE)

get_filename_component(APP_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/Code ABSOLUTE)
					   

get_filename_component(GAME_SRC_DIR
                       ${CMAKE_SOURCE_DIR}/Code ABSOLUTE)


get_filename_component(ASSET_DIR
					  ${CMAKE_SOURCE_DIR}/Assets
                      ABSOLUTE)					   

get_filename_component(ASSET_SHADER_DIR
					  ${CMAKE_SOURCE_DIR}/Assets/shaders
					  ABSOLUTE)		

get_filename_component(EditorIcons
					  ${CMAKE_SOURCE_DIR}/MapleEngine/Assets/editor-icons
                      ABSOLUTE)					   
					   
get_filename_component(Shaders
					  ${CMAKE_SOURCE_DIR}/MapleEngine/Assets/shaders
                      ABSOLUTE)		

get_filename_component(Toolchains
					  ${CMAKE_SOURCE_DIR}/MapleEngine/Toolchains
                      ABSOLUTE)	
					   
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")


message(${CMAKE_BUILD_TYPE})

if(ENGINE_AS_LIBRARY)
	add_definitions(-DMAPLE_DYNAMIC)
endif()

if(MAPLE_OPENGL)
	add_definitions(-DMAPLE_DYNAMIC)
endif()

if(MAPLE_VULKAN)
	add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)
	add_definitions(-DMAPLE_VULKAN)
endif()



add_subdirectory(${CMAKE_SOURCE_DIR}/MapleEngine/Code/Maple)
add_subdirectory(${CMAKE_SOURCE_DIR}/MapleEngine/Code/Scripts)

add_subdirectory(Assets)

file(GLOB METEOR_APP_SRC
	${GAME_SRC_DIR}/*.cpp
	${GAME_SRC_DIR}/*.h	
	${GAME_SRC_DIR}/Map/*.cpp
	${GAME_SRC_DIR}/Map/*.h
)

file(GLOB METEOR_EDITOR_SRC
	${GAME_SRC_DIR}/Editor/*.cpp
	${GAME_SRC_DIR}/Editor/*.h	
)

file(GLOB_RECURSE SHADERS_GLSL 
	${ASSET_SHADER_DIR}/*.vert 
	${ASSET_SHADER_DIR}/*.frag 
	${ASSET_SHADER_DIR}/*.comp 
	${ASSET_SHADER_DIR}/*.geom 
	${ASSET_SHADER_DIR}/*.tesc 
	${ASSET_SHADER_DIR}/*.tese 
	${ASSET_SHADER_DIR}/*.mesh 
	${ASSET_SHADER_DIR}/*.task 
	${ASSET_SHADER_DIR}/*.rgen
	${ASSET_SHADER_DIR}/*.rchit 
	${ASSET_SHADER_DIR}/*.rmiss
	${ASSET_SHADER_DIR}/*.shader
	${ASSET_SHADER_DIR}/*.glsl
)

foreach(FILE ${SHADERS_GLSL})
	MESSAGE(STATUS ${FILE})
endforeach()


if(${BuildType} MATCHES "Editor")
	add_definitions(-DBUILD_EDITOR)
	add_subdirectory(${CMAKE_SOURCE_DIR}/MapleEngine/Code/Editor)
endif()

if (${Target} MATCHES "Windows")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_compile_options("/std:c++17")

add_definitions(
	-D_CRT_SECURE_NO_WARNINGS 
	-D_CRT_RAND_S 
	-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 
	-DPLATFORM_WINDOWS -DPLATFORM_DESKTOP
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Maple")

source_group(TREE ${APP_SRC_DIR} FILES ${METEOR_APP_SRC} ${METEOR_EDITOR_SRC})

source_group(TREE ${ASSET_DIR} FILES ${SHADERS_GLSL})

if(${BuildType} MATCHES "Editor")
	add_executable(Meteor ${METEOR_APP_SRC} ${METEOR_EDITOR_SRC} ${SHADERS_GLSL})
	target_link_libraries(
		Meteor 
		MapleEngine
		EditorStatic
	)

else()
	add_executable(Meteor ${METEOR_APP_SRC})
	target_link_libraries(
		Meteor 
		MapleEngine
	)
endif()


target_include_directories(Meteor PUBLIC
	${GAME_SRC_DIR}
)

set_property(TARGET Meteor  PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${ASSET_DIR})

set_target_properties(Meteor PROPERTIES COMPILE_FLAGS "/MP /wd4819 /arch:SSE ")


string(REPLACE "/" "\\" RUNTIME_PATH ${ASSET_DIR}/)



add_custom_command(TARGET Meteor
				   POST_BUILD
	COMMAND powershell Copy-Item ${EditorIcons} ${RUNTIME_PATH} -recurse -Force
	COMMENT "Copying editor icons...."
)


#add_custom_command(TARGET Meteor
#				   POST_BUILD
#	COMMAND powershell Copy-Item ${Shaders} ${RUNTIME_PATH} -recurse -Force
#	COMMENT "Copying shaders...."
#)

#add_custom_command(TARGET Meteor
#				   POST_BUILD
#	COMMAND powershell Copy-Item ${ASSET_DIR}/${CMAKE_BUILD_TYPE}/MapleLibrary.dll ${RUNTIME_PATH} -Force
#	COMMENT "POST_BUILD Copying MapleLibrary.dll "
#)

#add_custom_command(TARGET Meteor
#				   POST_BUILD
#	COMMAND powershell Copy-Item ${ASSET_DIR}/${CMAKE_BUILD_TYPE}/MapleEngine.dll ${RUNTIME_PATH} -Force
#	COMMENT "POST_BUILD Copying MapleEngine.dll "
#)

add_custom_command(TARGET Meteor
				   POST_BUILD
	COMMAND powershell Copy-Item ${Toolchains} ${CMAKE_SOURCE_DIR} -recurse -Force
	COMMENT "POST_BUILD Copying Toolchains.... "
)

set_target_properties(MapleLibrary MapleAssembly PROPERTIES FOLDER ScriptExtension)


SET(LIBRARY_OUTPUT_PATH ${ASSET_DIR})
#SET_TARGET_PROPERTIES(MapleEngine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${ASSET_DIR})

endif()


