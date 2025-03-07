idf_component_register(
    SRCS "master.c"
    INCLUDE_DIRS "."
    REQUIRES mqtt esp_wifi nvs_flash esp_event
)