# The generated settings class, shared by both effects, the settings dialog and the tests
add_library(lightlyshaders_config STATIC)
kconfig_add_kcfg_files(lightlyshaders_config lightlyshaders_config.kcfgc)
set_target_properties(lightlyshaders_config PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(lightlyshaders_config PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lightlyshaders_config PUBLIC KF6::ConfigGui)

add_subdirectory(kcm)

add_library(kwin_effect_lightlyshaders MODULE
    lightlyshaders.cpp
    lightlyshaders.qrc
)

target_link_libraries(kwin_effect_lightlyshaders PRIVATE
    lshelper
)

install(TARGETS kwin_effect_lightlyshaders DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
