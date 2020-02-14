file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${retask_SOURCE_DIR}/include/*.h")
add_library(retask_library task.c worker.c utils.c ${HEADER_LIST})
target_include_directories(retask_library PUBLIC ../include)

source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${HEADER_LIST})