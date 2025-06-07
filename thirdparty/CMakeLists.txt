include (Compiler)

# Only add grpc subdirectory if BOND_ENABLE_GRPC is ON and gRPC is not provided by Conan
if(BOND_ENABLE_GRPC)
    if(NOT TARGET gRPC::grpc++) # Check if Conan's gRPC target does NOT exist
        # Add compile options specific to building Bond with submodule gRPC
        cxx_add_compile_options (Clang -Wno-unused-value)

        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/grpc/CMakeLists.txt")
            add_subdirectory(grpc)
            message(STATUS "Using gRPC submodule from thirdparty/grpc")
        else()
            message(WARNING "BOND_ENABLE_GRPC is ON, gRPC target from Conan not found, AND gRPC submodule not found at ${CMAKE_CURRENT_SOURCE_DIR}/grpc.")
        endif()
    else()
        message(STATUS "gRPC is being provided by Conan. Skipping thirdparty/grpc submodule.")
    endif()
endif()
