if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt")
    
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/install_manifest.txt" install_manifest)
    string(REGEX REPLACE "[\r\n]" ";" install_manifest "${install_manifest}")

    foreach(file ${install_manifest})
        if(EXISTS "${file}")
            message(STATUS "Uninstalling ${file}")        
            file(REMOVE ${file})
            if (EXISTS "${file}")
                message(FATAL_ERROR "Problem removing ${file}, check your permissions")
            endif()
        endif()
    endforeach()

endif()