set(kwin_lightlyshaders_blur_config_SRCS blur_config.cpp)
ki18n_wrap_ui(kwin_lightlyshaders_blur_config_SRCS blur_config.ui)

qt_add_dbus_interface(kwin_lightlyshaders_blur_config_SRCS ${KWIN_EFFECTS_INTERFACE} kwineffects_interface)

add_library(kwin_lightlyshaders_blur_config MODULE ${kwin_lightlyshaders_blur_config_SRCS})
target_link_libraries(kwin_lightlyshaders_blur_config PRIVATE
    Qt6::DBus
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils

    lightlyshaders_blurconfig
)

install(TARGETS kwin_lightlyshaders_blur_config DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs)
