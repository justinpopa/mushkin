# World module - Core document and world state management
# Includes Lua API (previously separate scripting module - merged to eliminate circular dependency)
add_library(world STATIC
    color_utils.h
    world_document.cpp
    world_document.h
    world_triggers.cpp
    world_trigger_matching.cpp
    world_trigger_execution.cpp
    world_alias_execution.cpp
    world_timers.cpp
    world_sendto.cpp
    world_logging.cpp
    world_protocol.cpp
    world_variables.cpp
    world_serialization.cpp
    world_document_plugins.cpp
    world_sound.cpp          # Sound system
    world_notepad.cpp        # Notepad window tracking
    world_recall.cpp         # Recall/buffer search
    world_mxp.cpp            # MXP (MUD eXtension Protocol)
    mxp_types.h              # MXP data structures
    accelerator_manager.h    # Keyboard accelerator management
    accelerator_manager.cpp
    macro_keypad_compat.h    # Macro/keypad compatibility with original MUSHclient
    macro_keypad_compat.cpp
    config_options.cpp
    config_options.h
    xml_serialization.cpp
    xml_serialization.h
    # MiniWindow (moved from ui to eliminate circular dependency)
    miniwindow.cpp
    miniwindow.h
    hotspot.h
    # NotepadWidget (moved from ui to eliminate circular dependency)
    notepad_widget.cpp
    notepad_widget.h
    # Lua scripting engine
    script_engine.h
    script_engine.cpp
    script_engine_callbacks.cpp
    lua_utils.cpp
    lua_dialog_callbacks.h
    lua_dialog_callbacks.cpp
    # Lua API modules
    lua_api/lua_registration.cpp
    lua_api/lua_constants.cpp
    lua_api/world_output.cpp
    lua_api/world_network.cpp
    lua_api/world_commands.cpp
    lua_api/world_variables.cpp
    lua_api/world_colors.cpp
    lua_api/world_utilities.cpp
    lua_api/world_settings.cpp
    lua_api/lua_progress.cpp
    lua_api/world_triggers.cpp
    lua_api/world_aliases.cpp
    lua_api/world_timers.cpp
    lua_api/world_logging.cpp
    lua_api/world_database.cpp
    lua_api/world_plugins.cpp
    lua_api/world_miniwindows.cpp
    lua_api/world_info.cpp
    lua_api/world_sounds.cpp        # Sound system API
    lua_api/world_notepads.cpp      # Notepad window API
    lua_api/world_arrays.cpp        # Named arrays API
    lua_api/lrexlib.cpp             # rex PCRE regex library
)

target_link_libraries(world PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    Qt6::Multimedia     # Sound playback
    Qt6::SpatialAudio   # Stereo panning support
    network
    automation
    third-party         # third-party defines lsqlite3 symbols (Lua SQLite binding)
    Lua::Lua            # LuaJIT for scripting engine
    yue_static          # YueScript transpiler (statically linked)
    ZLIB::ZLIB
    PCRE::PCRE          # PCRE regex library for rex Lua module
)

target_include_directories(world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/lua_api
)
