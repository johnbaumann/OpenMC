file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.S)
idf_component_register(
	SRCS ${SOURCE_FILES}
    INCLUDE_DIRS .
    REQUIRES app_update esp_http_server esp_wifi fatfs nvs_flash pthread spiffs)

# Bare metal stuff
idf_build_set_property(COMPILE_DEFINITIONS "-DXT_INTEXC_HOOKS" APPEND)
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_my_startup_file")
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_core0_stall_highint_hdl")
