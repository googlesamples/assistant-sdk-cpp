cmake_minimum_required(VERSION 3.10.2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "MinSizeRel" CACHE STRING "Choose the type of build, options are: Debug, Release, or MinSizeRel." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to MinSizeRel.")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

if(NOT BUILD_NUMBER)
    set(BUILD_NUMBER 0)
endif()
set(ASSISTANT_SDK_CPP_VERSION 0.0.1.${BUILD_NUMBER})

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
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
include(FindThreads)

include(googleapis)

protobuf_generate_grpc_cpp(
    ${PROTO_BASE_PATH}/google/api/http.proto
    ${PROTO_BASE_PATH}/google/api/annotations.proto
    ${PROTO_BASE_PATH}/google/type/latlng.proto
    ${PROTO_BASE_PATH}/google/assistant/embedded/v1alpha2/embedded_assistant.proto
)
set(GOOGLEAPIS_CCS
    ${_gRPC_PROTO_GENS_DIR}/google/api/http.grpc.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/api/http.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/api/annotations.grpc.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/api/annotations.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/type/latlng.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/type/latlng.grpc.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/assistant/embedded/v1alpha2/embedded_assistant.pb.cc
    ${_gRPC_PROTO_GENS_DIR}/google/assistant/embedded/v1alpha2/embedded_assistant.grpc.pb.cc
)
set_source_files_properties(${GOOGLEAPIS_CCS} PROPERTIES GENERATED TRUE)
include_directories(${_gRPC_PROTO_GENS_DIR})

set(CORE_SRCS src/base64_encode.cc)
set(AUDIO_INPUT_FILE_SRCS src/audio_input_file.cc)
set(ASSISTANT_AUDIO_SRCS src/run_assistant_audio.cc src/audio_pa.cc)
set(ASSISTANT_FILE_SRCS src/run_assistant_file.cc)
set(ASSISTANT_TEXT_SRCS src/run_assistant_text.cc)

include_directories(
    src
    ${CMAKE_CURRENT_BINARY_DIR}/grpc_ext-prefix/src/grpc_ext/third_party/abseil-cpp
)

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
    grpc++ grpc address_sorting upb cares gpr
    ${_ABSL_LIBRARIES} ${_PROTOBUF_LIBRARIES}
    ${_ZLIB_LIBRARIES} ${_SSL_LIBRARIES}
    ${_ALLTARGETS_LIBRARIES}
)

if(NOT HAVE_GETOPT_C)
    set(EXT_DEPS gperf_ext)
endif()
set(EXT_DEPS ${EXT_DEPS} portaudio_ext)

add_executable(run_assistant_text ${GOOGLEAPIS_CCS} ${CORE_SRCS} ${ASSISTANT_TEXT_SRCS})
target_link_libraries(run_assistant_text ${LDFLAGS})
add_dependencies(run_assistant_text ${EXT_DEPS})

add_executable(run_assistant_file ${GOOGLEAPIS_CCS} ${CORE_SRCS} ${AUDIO_INPUT_FILE_SRCS} ${ASSISTANT_FILE_SRCS})
target_link_libraries(run_assistant_file ${LDFLAGS})
add_dependencies(run_assistant_file ${EXT_DEPS})

add_executable(run_assistant_audio ${GOOGLEAPIS_CCS} ${CORE_SRCS} ${AUDIO_SRCS} ${ASSISTANT_AUDIO_SRCS})
target_link_libraries(run_assistant_audio ${LDFLAGS} ${_PORTAUDIO_LIB})
add_dependencies(run_assistant_audio ${EXT_DEPS})

if(APPLE)
    target_link_libraries(run_assistant_text "-framework CoreFoundation")
    target_link_libraries(run_assistant_file "-framework CoreFoundation")
    target_link_libraries(run_assistant_audio "-framework CoreFoundation")        
endif()

install (TARGETS run_assistant_audio run_assistant_text run_assistant_file 
    RUNTIME DESTINATION bin
)

add_custom_target(uninstall "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/cmake/make_uninstall.cmake")
