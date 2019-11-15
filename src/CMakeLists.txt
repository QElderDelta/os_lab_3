find_package(TBB CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(lab3
    main.cpp)

target_include_directories(lab3 PRIVATE ${TBB_IMPORTED_TARGETS})
target_link_libraries(lab3 PRIVATE ${TBB_IMPORTED_TARGETS})
target_link_libraries(lab3 ${CMAKE_THREAD_LIBS_INIT})
