FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(SRCS ${app_sources}
                       EMBED_TXTFILES "../LICENSE")

target_compile_definitions(${COMPONENT_LIB} PUBLIC
    CALENDAR_VERSION_MAJOR=1
    CALENDAR_VERSION_MINOR=0
    CALENDAR_VERSION_BUILD=0
)