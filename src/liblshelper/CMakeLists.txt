# Code shared by both effects, linked into each of them
add_library(lshelper STATIC lshelper.cpp)

set_target_properties(lshelper PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(lshelper PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(lshelper PUBLIC
    Qt6::Gui
    KWin::kwin
    lightlyshaders_config
)
