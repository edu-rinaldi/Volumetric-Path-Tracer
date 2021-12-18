add_executable(ypathtrace  ypathtrace.cpp)

set_target_properties(ypathtrace PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
target_include_directories(ypathtrace  PRIVATE ${CMAKE_SOURCE_DIR}/libs)
target_link_libraries(ypathtrace yocto yocto_pathtrace)

if(YOCTO_OPENGL)
target_link_libraries(ypathtrace yocto_gui)
endif(YOCTO_OPENGL)
