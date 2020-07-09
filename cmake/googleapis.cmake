# Set grpc plugin
if(NOT _gRPC_CPP_PLUGIN)
    set(_gRPC_CPP_PLUGIN ${CMAKE_STAGING_PREFIX}/bin/grpc_cpp_plugin${CMAKE_EXECUTABLE_SUFFIX})
endif()

if(NOT _gRPC_PROTO_GENS_DIR)
    set(_gRPC_PROTO_GENS_DIR ${CMAKE_CURRENT_BINARY_DIR}/gens)
endif()

if(NOT _gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR)
    set(_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR ${CMAKE_STAGING_PREFIX}/include)
endif()

# Normalize all paths
file(TO_NATIVE_PATH ${_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR} NATIVE_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR)
file(TO_NATIVE_PATH ${_gRPC_CPP_PLUGIN} NATIVE_gRPC_CPP_PLUGIN)
file(TO_NATIVE_PATH ${_gRPC_PROTO_GENS_DIR} NATIVE_gRPC_PROTO_GENS_DIR)
file(TO_NATIVE_PATH ${PROTO_BASE_PATH} NATIVE_PROTO_BASE_PATH)

# Create directory for generated .proto files
file(MAKE_DIRECTORY ${NATIVE_gRPC_PROTO_GENS_DIR})

#  protobuf_generate_grpc_cpp
#  --------------------------
#
#   Add custom commands to process ``.proto`` files to C++ using protoc and
#   GRPC plugin::
#
#     protobuf_generate_grpc_cpp [<ARGN>...]
#
#   ``ARGN``
#     ``.proto`` files
#
function(protobuf_generate_grpc_cpp)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_GRPC_CPP() called without any proto files")
    return()
  endif()

  set(_protobuf_include_path -I . -I ${NATIVE_gRPC_PROTOBUF_WELLKNOWN_INCLUDE_DIR})

  foreach(FIL ${ARGN})

    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    file(RELATIVE_PATH REL_FIL ${PROTO_BASE_PATH} ${ABS_FIL})
    get_filename_component(REL_DIR ${REL_FIL} DIRECTORY)

    set(RELFIL_WE ${_gRPC_PROTO_GENS_DIR}/${REL_DIR}/${FIL_WE})

    # Normalize paths
    file(TO_NATIVE_PATH ${RELFIL_WE} NATIVE_RELFIL_WE)
    file(TO_NATIVE_PATH ${REL_FIL} NATIVE_REL_FIL)

    add_custom_command(
      OUTPUT ${NATIVE_RELFIL_WE}.grpc.pb.cc
             ${NATIVE_RELFIL_WE}.grpc.pb.h
             ${NATIVE_RELFIL_WE}_mock.grpc.pb.h
             ${NATIVE_RELFIL_WE}.pb.cc
             ${NATIVE_RELFIL_WE}.pb.h
      COMMAND ${Protobuf_PROTOC_EXECUTABLE}
      ARGS --grpc_out=generate_mock_code=true:${NATIVE_gRPC_PROTO_GENS_DIR}
           --plugin=protoc-gen-grpc=${NATIVE_gRPC_CPP_PLUGIN}
           --cpp_out=${NATIVE_gRPC_PROTO_GENS_DIR}
           ${_protobuf_include_path}
           ${NATIVE_REL_FIL}
      DEPENDS ${ABS_FIL} grpc_ext
      WORKING_DIRECTORY ${PROTO_BASE_PATH}
      COMMENT "Running gRPC C++ protocol buffer compiler on ${FIL}"
      VERBATIM)
  endforeach()
endfunction()
