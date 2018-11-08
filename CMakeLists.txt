cmake_minimum_required(VERSION 3.11)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to MinSizeRel.")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

if(NOT BUILD_NUMBER)
    set(BUILD_NUMBER 0)
endif()
set(ASSISTANT_SDK_CPP_VERSION 0.1.${BUILD_NUMBER})

set(PACKAGE_NAME assistant-sdk-cpp)
project(${PACKAGE_NAME} VERSION "${ASSISTANT_SDK_CPP_VERSION}" LANGUAGES CXX C)

message(STATUS "Generator .............. ${CMAKE_GENERATOR}")
message(STATUS "Build Type ............. ${CMAKE_BUILD_TYPE}")
include (target_arch)
get_target_arch(TARGET_ARCH)
message(STATUS "Target ................. ${TARGET_ARCH}")

include(build_dependencies)

set(CMAKE_CXX_STANDARD_REQUIRED 11)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
include(FindThreads)


set(GOOGLEAPIS_API_CCS
    ${GOOGLEAPIS_GENS_PATH}/google/api/http.grpc.pb.cc
    ${GOOGLEAPIS_GENS_PATH}/google/api/http.pb.cc
    ${GOOGLEAPIS_GENS_PATH}/google/api/annotations.grpc.pb.cc
    ${GOOGLEAPIS_GENS_PATH}/google/api/annotations.pb.cc
)
set(GOOGLEAPIS_TYPE_CCS
    ${GOOGLEAPIS_GENS_PATH}/google/type/latlng.pb.cc
    ${GOOGLEAPIS_GENS_PATH}/google/type/latlng.grpc.pb.cc
)
set(GOOGLEAPIS_RPC_CCS
)
set(GOOGLEAPIS_ASSISTANT_PATH google/assistant/embedded/v1alpha2)
set(GOOGLEAPIS_ASSISTANT_CCS
    ${GOOGLEAPIS_GENS_PATH}/${GOOGLEAPIS_ASSISTANT_PATH}/embedded_assistant.pb.cc
    ${GOOGLEAPIS_GENS_PATH}/${GOOGLEAPIS_ASSISTANT_PATH}/embedded_assistant.grpc.pb.cc
)
set(GOOGLEAPIS_CCS
    ${GOOGLEAPIS_ASSISTANT_CCS} ${GOOGLEAPIS_API_CCS}
    ${GOOGLEAPIS_RPC_CCS} ${GOOGLEAPIS_TYPE_CCS}
)
set_source_files_properties(${GOOGLEAPIS_CCS} PROPERTIES GENERATED TRUE)

set(CORE_SRCS src/base64_encode.cc src/json_util.cc)
set(AUDIO_INPUT_FILE_SRCS src/audio_input_file.cc)
set(ASSISTANT_AUDIO_SRCS src/run_assistant_audio.cc src/audio_pa.cc)
set(ASSISTANT_FILE_SRCS src/run_assistant_file.cc)
set(ASSISTANT_TEXT_SRCS src/run_assistant_text.cc)


include_directories(src)


set(EXE_DEPS googleapis_ccs portaudio_ext)

if(WIN32 AND MSVC)
    add_definitions(-D_WIN32_WINNT=0x600)
    set(_ALLTARGETS_LIBRARIES ${_GPERF_LIBRARY})
    list(APPEND EXE_DEPS gperf_ext portaudio_ext)
elseif(APPLE)
    set(_ALLTARGETS_LIBRARIES ${CMAKE_DL_LIBS} m ${CMAKE_THREAD_LIBS_INIT})
elseif(ANDROID)
    set(_ALLTARGETS_LIBRARIES ${CMAKE_DL_LIBS} m log)
else()
    set(_ALLTARGETS_LIBRARIES ${CMAKE_DL_LIBS} rt m ${CMAKE_THREAD_LIBS_INIT})
endif()


set(LDFLAGS
    grpc++ grpc gpr grpc_cronet address_sorting
    ${_PROTOBUF_LIBRARIES} ${_CARES_LIBRARIES}
    ${_ZLIB_LIBRARIES} ${_SSL_LIBRARIES}
    ${_ALLTARGETS_LIBRARIES}
)


add_library(googleapis_ccs STATIC ${GOOGLEAPIS_CCS})
add_dependencies(googleapis_ccs googleapis)

add_executable(run_assistant_text ${CORE_SRCS} ${ASSISTANT_TEXT_SRCS})
target_link_libraries(run_assistant_text googleapis_ccs ${LDFLAGS})
add_dependencies(run_assistant_text ${EXE_DEPS})

add_executable(run_assistant_file ${CORE_SRCS} ${AUDIO_INPUT_FILE_SRCS} ${ASSISTANT_FILE_SRCS})
target_link_libraries(run_assistant_file googleapis_ccs ${LDFLAGS})
add_dependencies(run_assistant_file ${EXE_DEPS})

add_executable(run_assistant_audio ${CORE_SRCS} ${AUDIO_SRCS} ${ASSISTANT_AUDIO_SRCS})
target_link_libraries(run_assistant_audio googleapis_ccs ${LDFLAGS} ${_PORTAUDIO_LIB})
add_dependencies(run_assistant_audio ${EXE_DEPS})


install (TARGETS run_assistant_audio run_assistant_text run_assistant_file 
        RUNTIME DESTINATION bin
)

add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/cmake/make_uninstall.cmake")
