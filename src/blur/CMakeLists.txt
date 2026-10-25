# The generated settings class, shared by the effect, the settings dialog and the tests
add_library(lightlyshaders_blurconfig STATIC)
kconfig_add_kcfg_files(lightlyshaders_blurconfig blurconfig.kcfgc)
set_target_properties(lightlyshaders_blurconfig PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(lightlyshaders_blurconfig PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lightlyshaders_blurconfig PUBLIC KF6::ConfigGui)

add_subdirectory(kcm)

add_library(lightlyshaders_blur MODULE
    blur.cpp
    blur.qrc
    main.cpp
)
target_link_libraries(lightlyshaders_blur PRIVATE
    KWin::kwin

    XCB::XCB
    KDecoration3::KDecoration

    lightlyshaders_blurconfig
    lshelper
)

install(TARGETS lightlyshaders_blur DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
