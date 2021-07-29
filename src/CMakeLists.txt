file(GLOB SOURCE_FILES *.c *.cc)
idf_component_register(
	SRCS ${SOURCE_FILES}
    INCLUDE_DIRS ../include
    REQUIRES fatfs nvs_flash esp_wifi)