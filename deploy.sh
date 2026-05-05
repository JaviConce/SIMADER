#!/bin/bash

# Directorio base del proyecto
BASE_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "Lanzando terminal con 3 tabs..."

osascript <<EOF
tell application "Terminal"
    activate

    -- Crear primera ventana con Node-RED
    do script "cd '$BASE_DIR' && echo '=== NODE-RED ===' && ./node-red/start.sh"
    delay 1

    -- Crear nueva tab para ESP32 Device
    tell application "System Events"
        keystroke "t" using {command down}
    end tell
    delay 0.5
    do script "cd '$BASE_DIR/ESP32_App_Device' && echo '=== ESP32 DEVICE ===' && ./run.sh" in selected tab of front window

    -- Crear nueva tab para ESP32 Manager
    tell application "System Events"
        keystroke "t" using {command down}
    end tell
    delay 0.5
    do script "cd '$BASE_DIR/ESP32_App_Manager' && echo '=== ESP32 MANAGER ===' && ./run.sh" in selected tab of front window

end tell
EOF

echo "Terminal con 3 tabs lanzada."
