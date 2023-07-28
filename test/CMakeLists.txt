add_executable(mycamera main.cpp)
target_link_libraries(
    mycamera
    ${OpenCV_LIBS}
    mycameracore
)
