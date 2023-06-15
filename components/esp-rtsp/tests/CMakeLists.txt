cmake_minimum_required(VERSION 3.10)

# set the project name
project(rtsp_test)

# add the executable
add_executable(rtsp_test "main.c" "../rtsp-server.c")