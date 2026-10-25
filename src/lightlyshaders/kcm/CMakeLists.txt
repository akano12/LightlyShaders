set(kwin_lightlyshaders_config_SOURCES lightlyshaders_kcm.cpp)

ki18n_wrap_ui(kwin_lightlyshaders_config_SOURCES lightlyshaders_config.ui)

qt_add_dbus_interface(kwin_lightlyshaders_config_SOURCES ${KWIN_EFFECTS_INTERFACE} kwineffects_interface)

add_library(kwin_lightlyshaders_config MODULE ${kwin_lightlyshaders_config_SOURCES})

target_link_libraries(kwin_lightlyshaders_config PRIVATE
    Qt6::DBus
    Qt6::Widgets
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::WidgetsAddons

    lightlyshaders_config
)

install(TARGETS kwin_lightlyshaders_config DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/configs)
