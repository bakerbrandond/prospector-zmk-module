zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library()

if(CONFIG_SHIELD_PROSPECTOR_ADAPTER)

        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/modules/lvgl)

        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_CAPS_WORD_ENABLED)
                zephyr_library_sources(src/events/caps_word_state_changed.c)
                set_source_files_properties(
                        ${APPLICATION_SOURCE_DIR}/src/behaviors/behavior_caps_word.c
                        TARGET_DIRECTORY app
                        PROPERTIES HEADER_FILE_ONLY ON)
                target_sources(app PRIVATE src/behaviors/behavior_caps_word.c)
        endif()

        zephyr_library_sources(src/events/split_central_status_changed.c)
        zephyr_library_sources(src/split/bluetooth/central_status_changed_observer.c)

        # Built whenever the keymap has &zappy/&zflap nodes, so with the game
        # off they are inert no-ops instead of falling through to lower layers.
        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_ZAPPY_TOGGLE_ENABLED OR CONFIG_DT_HAS_ZMK_BEHAVIOR_ZAPPY_FLAP_ENABLED)
                zephyr_library_sources(src/behaviors/behavior_zappy.c)
        endif()

        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_PROSPECTOR_BACKLIGHT_ENABLED)
                zephyr_library_sources(src/behaviors/behavior_prospector_bl.c)
        endif()

        if(CONFIG_PROSPECTOR_MATRIX_SCREENSAVER)
                zephyr_library_include_directories(boards/shields/prospector_adapter/include)
                zephyr_library_sources(src/matrix/matrix_core.c)
                zephyr_library_sources(src/matrix/matrix_screen.c)
                zephyr_library_sources(src/matrix/matrix_mode.c)
        endif()

        if(CONFIG_PROSPECTOR_ZAPPY_BIRDS)
                zephyr_library_include_directories(boards/shields/prospector_adapter/include)
                zephyr_library_sources(src/zappy/zappy_core.c)
                zephyr_library_sources(src/zappy/zappy_screen.c)
                zephyr_library_sources(src/zappy/zappy_mode.c)
                zephyr_library_sources(src/zappy/zappy_store.c)
        endif()

endif()