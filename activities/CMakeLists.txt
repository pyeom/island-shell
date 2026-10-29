# Activities that ship with the shell. They only use the public IslandShell
# API, same as user activities.

qt_add_qml_module(islandactivities
    URI IslandShell.Activities
    VERSION 0.1
    STATIC
    OUTPUT_DIRECTORY ${ISLAND_QML_OUTPUT}/IslandShell/Activities
    IMPORTS IslandShell
    QML_FILES
        Idle.qml
        ClockText.qml
        IdleExpanded.qml
        WorkspaceDots.qml
        ShellErrors.qml
        Notification.qml
        NotificationCard.qml
        NotificationHistory.qml
        VolumeOsd.qml
        Bars.qml
        Media.qml
        MediaPanel.qml
        LiveActivity.qml
        Launcher.qml
        LauncherPanel.qml
        PowerMenu.qml
        PowerPanel.qml
        QuickSettings.qml
        QuickSettingsPanel.qml
        SystemTray.qml
        SystemTrayPanel.qml
        TrayBubble.qml
        AudioDeviceList.qml
        AudioStreamList.qml
        WeatherLine.qml
        SystemStatsRow.qml
        SystemHeat.qml
        RainAlert.qml
        TimerActivity.qml
        ScreenshotActivity.qml
        WallpaperPicker.qml
        EventForm.qml
        WifiList.qml
        BluetoothList.qml
        BatteryLow.qml
        BatteryText.qml
        PrivacyIndicator.qml
        CalendarMonth.qml
        EventList.qml
        EventSoon.qml
)

target_link_libraries(islandactivities PUBLIC islandshell)
