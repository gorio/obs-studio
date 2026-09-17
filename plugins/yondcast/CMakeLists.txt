cmake_minimum_required(VERSION 3.28...3.30)

find_package(Qt6 REQUIRED Widgets Network WebSockets)

add_library(yondcast MODULE)
add_library(OBS::yondcast ALIAS yondcast)

target_sources(
  yondcast
  PRIVATE
    yondcast-plugin.cpp
    yondcast-dock.cpp
    yondcast-dock.hpp
)

target_link_libraries(
  yondcast
  PRIVATE
    OBS::frontend-api
    OBS::libobs
    Qt::Widgets
    Qt::Network
    Qt::WebSockets
)

set_target_properties_obs(
  yondcast
  PROPERTIES
    FOLDER plugins/yondcast
    PREFIX ""
    AUTOMOC ON
)
