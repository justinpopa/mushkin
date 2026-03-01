# Network module
add_library(network STATIC
    world_socket.cpp
    world_socket.h
    # Remote Access Server (telnet server for remote client connections)
    i_remote_transport.h
    ansi_formatter.cpp
    ansi_formatter.h
    telnet_server_session.cpp
    telnet_server_session.h
    remote_client.cpp
    remote_client.h
    remote_access_server.cpp
    remote_access_server.h
    # SSH host key management
    ssh_host_key_manager.cpp
    ssh_host_key_manager.h
)

target_link_libraries(network PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    ZLIB::ZLIB  # Needed because world_socket.cpp includes world_document.h which uses zlib
    utils
    ssh
)

target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
