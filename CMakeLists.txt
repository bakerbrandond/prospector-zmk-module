zephyr_include_directories(include)
zephyr_include_directories(${APPLICATION_SOURCE_DIR}/include)
zephyr_library()

# Keyboard halves: report USB power to the dongle (see include/prospector/charging_service.h)
if(CONFIG_PROSPECTOR_PERIPHERAL_CHARGING_SERVICE)
        zephyr_library_sources(src/split/bluetooth/charging_peripheral.c)
endif()

if(CONFIG_SHIELD_PROSPECTOR_ADAPTER)

        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/drivers/display)
        add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/modules/lvgl)

        zephyr_library_sources(src/events/split_central_status_changed.c)
        zephyr_library_sources(src/split/bluetooth/central_status_changed_observer.c)

        if(CONFIG_PROSPECTOR_CHARGING_INDICATOR)
                zephyr_library_sources(src/events/peripheral_charging_changed.c)
                zephyr_library_sources(src/split/bluetooth/charging_central.c)
        endif()

        # Built whenever the keymap has &zappy/&zflap nodes, so with the game
        # off they are inert no-ops instead of falling through to lower layers.
        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_ZAPPY_TOGGLE_ENABLED OR CONFIG_DT_HAS_ZMK_BEHAVIOR_ZAPPY_FLAP_ENABLED)
                zephyr_library_sources(src/behaviors/behavior_zappy.c)
        endif()

        # Built whenever the keymap has &pet/&petkey nodes, so with the pet off
        # they are inert no-ops instead of falling through to lower layers.
        if(CONFIG_DT_HAS_ZMK_BEHAVIOR_PET_ENABLED OR CONFIG_DT_HAS_ZMK_BEHAVIOR_PET_KEY_ENABLED)
                zephyr_library_sources(src/behaviors/behavior_pet.c)
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

        if(CONFIG_PROSPECTOR_TYPING_PET)
                zephyr_library_include_directories(boards/shields/prospector_adapter/include)
                zephyr_library_sources(src/pet/pet_core.c)
                zephyr_library_sources(src/pet/pet_sprites.c)
                zephyr_library_sources(src/pet/pet_panel.c)
                zephyr_library_sources(src/pet/pet_mode.c)
                zephyr_library_sources(src/pet/pet_store.c)
        endif()

endif()