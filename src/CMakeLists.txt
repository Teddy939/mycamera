add_library(arducam_vcm SHARED IMPORTED)
set_target_properties(
    arducam_vcm
    PROPERTIES
    IMPORTED_LOCATION
    ${PROJECT_SOURCE_DIR}/lib/libarducam_vcm.so
)

add_library(
    mycameracore
    STATIC
    image_sensor.cpp
    focuser.cpp
    live_viewer.cpp
)
target_link_libraries(mycameracore ${OpenCV_LIBS} arducam_vcm)
