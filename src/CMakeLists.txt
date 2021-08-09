file(GLOB SOURCE_FILES *.c *.cc *.s)
idf_component_register(
	SRCS ${SOURCE_FILES}
    INCLUDE_DIRS ../include
    REQUIRES fatfs nvs_flash esp_wifi app_update pthread)
idf_build_set_property(COMPILE_DEFINITIONS "-DXT_INTEXC_HOOKS" APPEND)
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_my_startup_file")
target_link_libraries(${COMPONENT_TARGET} "-u ld_include_core0_stall_highint_hdl")