set(SRC_DIR "${SRC_DIR}")
set(DST_DIR "${DST_DIR}")
set(PROJECT_NAME "${PROJECT_NAME}")

set(BSA_SRC "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bsa")
if(EXISTS "${BSA_SRC}")
    message(STATUS "Copying ${BSA_SRC} to ${DST_DIR}")
    file(COPY "${BSA_SRC}" DESTINATION "${DST_DIR}")
endif()