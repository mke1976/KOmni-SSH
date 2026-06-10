#!/bin/bash

# --- 1. SETUP & CONFIGURATION ---
# Updated timestamp to reflect the latest revision
VERSION_TS="26-05-06_18:55"
TITLE_TEXT="<span font_desc='sans italic 9'>$VERSION_TS</span>"

CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}"
CONFIG_FILE="$CONFIG_DIR/omni-ssh.conf"

load_settings() {
    [ -f "$CONFIG_FILE" ] && source "$CONFIG_FILE"
    
    GLOBAL_SSH_PASS="${GLOBAL_SSH_PASS:-}"
    SHEET_CSV_URL="${SHEET_CSV_URL:-https://docs.google.com/spreadsheets/d/1n3QWceMh5V1npxnWZv8Z25BHoqJCuv_lVu26dbqabDM/export?format=csv}"
    N1_EN="${N1_EN:-ON}"; N1_NAME="${N1_NAME:-Ngrok PC 1}"; N1_USER="${N1_USER:-user}"; N1_API="${N1_API:-}"
    N2_EN="${N2_EN:-ON}"; N2_NAME="${N2_NAME:-Ngrok PC 2}"; N2_USER="${N2_USER:-user}"; N2_API="${N2_API:-}"
    V1_EN="${V1_EN:-ON}"; V1_NAME="${V1_NAME:-VPN PC 1}"; V1_USER="${V1_USER:-mk}"; V1_IP="${V1_IP:-192.168.2.50}"; V1_BRIDGE="${V1_BRIDGE:-WARP}"; V1_PRIV="${V1_PRIV:-fritz}"
    V2_EN="${V2_EN:-ON}"; V2_NAME="${V2_NAME:-VPN PC 2}"; V2_USER="${V2_USER:-mk}"; V2_IP="${V2_IP:-192.168.2.51}"; V2_BRIDGE="${V2_BRIDGE:-WARP}"; V2_PRIV="${V2_PRIV:-fritz}"
    V3_EN="${V3_EN:-ON}"; V3_NAME="${V3_NAME:-VPN PC 3}"; V3_USER="${V3_USER:-mk}"; V3_IP="${V3_IP:-192.168.2.52}"; V3_BRIDGE="${V3_BRIDGE:-WARP}"; V3_PRIV="${V3_PRIV:-fritz}"
    D1_EN="${D1_EN:-ON}"; D1_NAME="${D1_NAME:-Direct PC 1}"; D1_USER="${D1_USER:-user}"; D1_HOST="${D1_HOST:-home-nw-sgp.freeddns.org}"; D1_PORT="${D1_PORT:-10500}"
    D2_EN="${D2_EN:-ON}"; D2_NAME="${D2_NAME:-Direct PC 2}"; D2_USER="${D2_USER:-user}"; D2_HOST="${D2_HOST:-address.com}"; D2_PORT="${D2_PORT:-22}"
}

save_settings() {
    cat <<EOF > "$CONFIG_FILE"
GLOBAL_SSH_PASS="$GLOBAL_SSH_PASS"
SHEET_CSV_URL="$SHEET_CSV_URL"
N1_EN="$N1_EN"; N1_NAME="$N1_NAME"; N1_USER="$N1_USER"; N1_API="$N1_API"
N2_EN="$N2_EN"; N2_NAME="$N2_NAME"; N2_USER="$N2_USER"; N2_API="$N2_API"
V1_EN="$V1_EN"; V1_NAME="$V1_NAME"; V1_USER="$V1_USER"; V1_IP="$V1_IP"; V1_BRIDGE="$V1_BRIDGE"; V1_PRIV="$V1_PRIV"
V2_EN="$V2_EN"; V2_NAME="$V2_NAME"; V2_USER="$V2_USER"; V2_IP="$V2_IP"; V2_BRIDGE="$V2_BRIDGE"; V2_PRIV="$V2_PRIV"
V3_EN="$V3_EN"; V3_NAME="$V3_NAME"; V3_USER="$V3_USER"; V3_IP="$V3_IP"; V3_BRIDGE="$V3_BRIDGE"; V3_PRIV="$V3_PRIV"
D1_EN="$D1_EN"; D1_NAME="$D1_NAME"; D1_USER="$D1_USER"; D1_HOST="$D1_HOST"; D1_PORT="$D1_PORT"
D2_EN="$D2_EN"; D2_NAME="$D2_NAME"; D2_USER="$D2_USER"; D2_HOST="$D2_HOST"; D2_PORT="$D2_PORT"
EOF
    load_settings
}

load_settings

# Check for sshpass dependency[cite: 4]
if [ -n "$GLOBAL_SSH_PASS" ] && ! command -v sshpass &> /dev/null; then
    zenity --warning --text="sshpass not found. Install it for auto-password."
    USE_SSHPASS=false
else
    USE_SSHPASS=${GLOBAL_SSH_PASS:+true}
    USE_SSHPASS=${USE_SSHPASS:-false}
fi

# --- 2. STATUS POLLING LOGIC ---
declare -A PC_STATUS

fetch_status() {
    PC_STATUS=()
    if [ -n "$SHEET_CSV_URL" ]; then
        # Downloads CSV and extracts Name ($1) and Timestamp ($NF)[cite: 4]
        while IFS= read -r line; do
            [ -z "$line" ] && continue
            local name=$(echo "$line" | awk -F',' '{print $1}' | tr -d '"\r')
            local ts=$(echo "$line" | awk -F',' '{print $NF}' | tr -d '"\r')
            if [[ "$name" != "PC Name" && "$ts" =~ ^[0-9]+$ ]]; then
                PC_STATUS["$name"]="$ts"
            fi
        done <<< "$(curl -sL "$SHEET_CSV_URL")"
    fi
}

get_status_icon() {
    local target_name="$1"
    local ts="${PC_STATUS["$target_name"]}"
    if [ -z "$ts" ]; then
        echo "⚪" 
    else
        # 12-minute threshold for online status[cite: 4]
        (( $(date +%s) - ts <= 720 )) && echo "🟢" || echo "🔴"
    fi
}

# --- 3. BACKGROUND NOTIFIER SUBPROCESS ---
status_notifier_loop() {
    declare -A PREV_STATE
    local FIRST_RUN=true
    while true; do
        declare -A CURRENT_STATE
        if [ -n "$SHEET_CSV_URL" ]; then
            while IFS= read -r line; do
                [ -z "$line" ] && continue
                local name=$(echo "$line" | awk -F',' '{print $1}' | tr -d '"\r')
                local ts=$(echo "$line" | awk -F',' '{print $NF}' | tr -d '"\r')
                if [[ "$name" != "PC Name" && "$ts" =~ ^[0-9]+$ ]]; then
                    if (( $(date +%s) - ts <= 720 )); then
                        CURRENT_STATE["$name"]="online"
                    else
                        CURRENT_STATE["$name"]="offline"
                    fi
                fi
            done <<< "$(curl -sL "$SHEET_CSV_URL")"
        fi

        # Logic to suppress notifications on script startup[cite: 4]
        if [ "$FIRST_RUN" = true ]; then
            for pc in "${!CURRENT_STATE[@]}"; do
                PREV_STATE["$pc"]="${CURRENT_STATE[$pc]}"
            done
            FIRST_RUN=false
        else
            for pc in "${!CURRENT_STATE[@]}"; do
                if [[ "${CURRENT_STATE[$pc]}" == "online" ]] && [[ "${PREV_STATE[$pc]}" != "online" ]]; then
                    # Using hinted notify-send to ensure persistence in KDE notification history[cite: 4]
                    notify-send -i network-wired -a "Omni SSH" "Omni SSH" "$pc is now ONLINE" \
                        --hint=int:transient:0 --hint=string:desktop-entry:omni-ssh
                fi
                PREV_STATE["$pc"]="${CURRENT_STATE[$pc]}"
            done
        fi

        sleep 30
    done
}

# Start background notifier and ensure it is killed on main script exit[cite: 4]
status_notifier_loop &
NOTIFIER_PID=$!
trap "kill $NOTIFIER_PID 2>/dev/null; exit" EXIT

# --- 4. MAIN INTERFACE LOOP ---
while true; do
    # 1. Fetch data at launch and upon every return to main loop[cite: 4]
    fetch_status

    # 2. Build menu options based on enabled targets[cite: 4]
    MENU_OPTIONS=()
    [ "$N1_EN" == "ON" ] && MENU_OPTIONS+=("N1" "Ngrok" "$N1_NAME" "$(get_status_icon "$N1_NAME")")
    [ "$N2_EN" == "ON" ] && MENU_OPTIONS+=("N2" "Ngrok" "$N2_NAME" "$(get_status_icon "$N2_NAME")")
    [ "$V1_EN" == "ON" ] && MENU_OPTIONS+=("V1" "Double VPN" "$V1_NAME" "$(get_status_icon "$V1_NAME")")
    [ "$V2_EN" == "ON" ] && MENU_OPTIONS+=("V2" "Double VPN" "$V2_NAME" "$(get_status_icon "$V2_NAME")")
    [ "$V3_EN" == "ON" ] && MENU_OPTIONS+=("V3" "Double VPN" "$V3_NAME" "$(get_status_icon "$V3_NAME")")
    [ "$D1_EN" == "ON" ] && MENU_OPTIONS+=("D1" "Direct" "$D1_NAME" "$(get_status_icon "$D1_NAME")")
    [ "$D2_EN" == "ON" ] && MENU_OPTIONS+=("D2" "Direct" "$D2_NAME" "$(get_status_icon "$D2_NAME")")
    
    MENU_OPTIONS+=("S" "System" "Settings" "" "R" "System" "Refresh" "" "Q" "System" "Quit" "")

    # 3. Static Zenity List[cite: 4]
    CHOICE=$(zenity --list --title="Omni SSH Connector" \
        --text="$TITLE_TEXT" \
        --column="ID" --column="Type" --column="Target" --column="Status" \
        "${MENU_OPTIONS[@]}" --width=550 --height=580 --hide-column=1)

    [ $? -ne 0 ] || [ "$CHOICE" == "Q" ] || [ -z "$CHOICE" ] && break

    # --- 5. REFRESH LOGIC ---
    if [ "$CHOICE" == "R" ]; then
        continue
    fi

    # --- 6. SETTINGS MODULE ---
    if [ "$CHOICE" == "S" ]; then
        MASKED_PASS=$( [ -n "$GLOBAL_SSH_PASS" ] && echo "********" || echo "" )
        SUB_CHOICE=$(zenity --list --title="Settings" --column="Field" --column="Value" \
            "Global: Key Passphrase" "$MASKED_PASS" "Global: Sheet CSV URL" "$SHEET_CSV_URL" \
            "Ngrok 1: Show (ON/OFF)" "$N1_EN" "Ngrok 1: Name" "$N1_NAME" "Ngrok 1: User" "$N1_USER" "Ngrok 1: API" "$N1_API" \
            "Ngrok 2: Show (ON/OFF)" "$N2_EN" "Ngrok 2: Name" "$N2_NAME" "Ngrok 2: User" "$N2_USER" "Ngrok 2: API" "$N2_API" \
            "VPN 1: Show (ON/OFF)" "$V1_EN" "VPN 1: Name" "$V1_NAME" "VPN 1: User" "$V1_USER" "VPN 1: IP" "$V1_IP" "VPN 1: Bridge" "$V1_BRIDGE" "VPN 1: Private" "$V1_PRIV" \
            "VPN 2: Show (ON/OFF)" "$V2_EN" "VPN 2: Name" "$V2_NAME" "VPN 2: User" "$V2_USER" "VPN 2: IP" "$V2_IP" "VPN 2: Bridge" "$V2_BRIDGE" "VPN 2: Private" "$V2_PRIV" \
            "VPN 3: Show (ON/OFF)" "$V3_EN" "VPN 3: Name" "$V3_NAME" "VPN 3: User" "$V3_USER" "VPN 3: IP" "$V3_IP" "VPN 3: Bridge" "$V3_BRIDGE" "VPN 3: Private" "$V3_PRIV" \
            "Direct 1: Show (ON/OFF)" "$D1_EN" "Direct 1: Name" "$D1_NAME" "Direct 1: User" "$D1_USER" "Direct 1: Host" "$D1_HOST" "Direct 1: Port" "$D1_PORT" \
            "Direct 2: Show (ON/OFF)" "$D2_EN" "Direct 2: Name" "$D2_NAME" "Direct 2: User" "$D2_USER" "Direct 2: Host" "$D2_HOST" "Direct 2: Port" "$D2_PORT" \
            --width=600 --height=600)
        
        if [ $? -eq 0 ] && [ -n "$SUB_CHOICE" ]; then
            NEW_VAL=$(zenity --entry --title="Edit" --text="New value for $SUB_CHOICE:")
            [ $? -eq 0 ] && {
                case "$SUB_CHOICE" in
                    "Global: Key Passphrase") GLOBAL_SSH_PASS="$NEW_VAL" ;;
                    "Global: Sheet CSV URL") SHEET_CSV_URL="$NEW_VAL" ;;
                    "Ngrok 1: Show (ON/OFF)") N1_EN="$NEW_VAL" ;; "Ngrok 1: Name") N1_NAME="$NEW_VAL" ;; "Ngrok 1: User") N1_USER="$NEW_VAL" ;; "Ngrok 1: API") N1_API="$NEW_VAL" ;;
                    "Ngrok 2: Show (ON/OFF)") N2_EN="$NEW_VAL" ;; "Ngrok 2: Name") N2_NAME="$NEW_VAL" ;; "Ngrok 2: User") N2_USER="$NEW_VAL" ;; "Ngrok 2: API") N2_API="$NEW_VAL" ;;
                    "VPN 1: Show (ON/OFF)") V1_EN="$NEW_VAL" ;; "VPN 1: Name") V1_NAME="$NEW_VAL" ;; "VPN 1: User") V1_USER="$NEW_VAL" ;; "VPN 1: IP") V1_IP="$NEW_VAL" ;; "VPN 1: Bridge") V1_BRIDGE="$NEW_VAL" ;; "VPN 1: Private") V1_PRIV="$NEW_VAL" ;;
                    "VPN 2: Show (ON/OFF)") V2_EN="$NEW_VAL" ;; "VPN 2: Name") V2_NAME="$NEW_VAL" ;; "VPN 2: User") V2_USER="$NEW_VAL" ;; "VPN 2: IP") V2_IP="$NEW_VAL" ;; "VPN 2: Bridge") V2_BRIDGE="$NEW_VAL" ;; "VPN 2: Private") V2_PRIV="$NEW_VAL" ;;
                    "VPN 3: Show (ON/OFF)") V3_EN="$NEW_VAL" ;; "VPN 3: Name") V3_NAME="$NEW_VAL" ;; "VPN 3: User") V3_USER="$NEW_VAL" ;; "VPN 3: IP") V3_IP="$NEW_VAL" ;; "VPN 3: Bridge") V3_BRIDGE="$NEW_VAL" ;; "VPN 3: Private") V3_PRIV="$NEW_VAL" ;;
                    "Direct 1: Show (ON/OFF)") D1_EN="$NEW_VAL" ;; "Direct 1: Name") D1_NAME="$NEW_VAL" ;; "Direct 1: User") D1_USER="$NEW_VAL" ;; "Direct 1: Host") D1_HOST="$NEW_VAL" ;; "Direct 1: Port") D1_PORT="$NEW_VAL" ;;
                    "Direct 2: Show (ON/OFF)") D2_EN="$NEW_VAL" ;; "Direct 2: Name") D2_NAME="$NEW_VAL" ;; "Direct 2: User") D2_USER="$NEW_VAL" ;; "Direct 2: Host") D2_HOST="$NEW_VAL" ;; "Direct 2: Port") D2_PORT="$NEW_VAL" ;;
                esac
                save_settings
            }
        fi
        continue
    fi

    # --- 7. CONNECTION LOGIC ---
    case "$CHOICE" in
        "N1") T_LABEL="$N1_NAME" ;; "N2") T_LABEL="$N2_NAME" ;;
        "V1") T_LABEL="$V1_NAME" ;; "V2") T_LABEL="$V2_NAME" ;; "V3") T_LABEL="$V3_NAME" ;;
        "D1") T_LABEL="$D1_NAME" ;; "D2") T_LABEL="$D2_NAME" ;;
    esac

    APP_CHOICE_RAW=$(zenity --forms --title="Launch Action" --text="What do you want to open on $T_LABEL?" \
        --add-combo="Application" --combo-values="Terminal|Firefox|Dolphin|KWrite")
    
    [ $? -ne 0 ] || [ -z "$APP_CHOICE_RAW" ] && continue
    APP_CHOICE=$(echo "$APP_CHOICE_RAW" | tr -d '|')

    WP_PREFIX=()
    [ "$USE_SSHPASS" = true ] && WP_PREFIX+=(sshpass -P 'pass' -p "$GLOBAL_SSH_PASS")
    WP_PREFIX+=(waypipe -c lz4 --no-gpu ssh)

    if [[ "$CHOICE" == N* ]]; then
        [ "$CHOICE" == "N1" ] && { API="$N1_API"; USER="$N1_USER"; } || { API="$N2_API"; USER="$N2_USER"; }
        RESPONSE=$(curl -s -H "Authorization: Bearer $API" -H "ngrok-version: 2" https://api.ngrok.com/tunnels)
        URL=$(echo "$RESPONSE" | jq -r '.tunnels[] | select(.public_url | startswith("tcp://")) | .public_url' | head -n 1)
        ADDR=${URL#tcp://}
        PORT="${ADDR##*:}"; HOST="${ADDR%:*}"; TARGET="$USER@$HOST"
        OPTS=(-o "StrictHostKeyChecking=no" -o "UserKnownHostsFile=/dev/null" -p "$PORT")
        
    elif [[ "$CHOICE" == V* ]]; then
        [ "$CHOICE" == "V1" ] && { USER="$V1_USER"; IP="$V1_IP"; BRIDGE="$V1_BRIDGE"; PRIV="$V1_PRIV"; }
        [ "$CHOICE" == "V2" ] && { USER="$V2_USER"; IP="$V2_IP"; BRIDGE="$V2_BRIDGE"; PRIV="$V2_PRIV"; }
        [ "$CHOICE" == "V3" ] && { USER="$V3_USER"; IP="$V3_IP"; BRIDGE="$V3_BRIDGE"; PRIV="$V3_PRIV"; }
        
        (
            echo "10"; echo "# Checking IPv6 connectivity..."
            HAS_NATIVE_IPV6=false
            if [ -n "$(ip -6 addr show scope global)" ] && ping6 -c 1 -W 1 2001:4860:4860::8888 >/dev/null 2>&1; then
                HAS_NATIVE_IPV6=true; echo "30"; echo "# Native IPv6 detected! Skipping $BRIDGE..."
            else
                for attempt in {1..3}; do
                    echo $(( (attempt-1)*13 )); echo "# Connecting Bridge: $BRIDGE (Attempt $attempt/3)..."
                    nmcli connection up "$BRIDGE" >/dev/null 2>&1 & NM_PID=$!
                    START_TIME=$SECONDS
                    while [ $(( SECONDS - START_TIME )) -lt 10 ]; do
                        ping6 -c 1 -W 1 2001:4860:4860::8888 >/dev/null 2>&1 && { BRIDGE_SUCCESS=true; break 2; }
                        sleep 0.5
                    done
                    kill $NM_PID 2>/dev/null
                done
                [ "$BRIDGE_SUCCESS" = false ] && exit 1
            fi

            for attempt in {1..3}; do
                echo $(( 40 + (attempt-1)*15 )); echo "# Connecting Fritz: $PRIV (Attempt $attempt/3)..."
                nmcli connection up "$PRIV" >/dev/null 2>&1 & NM_PID=$!
                START_TIME=$SECONDS
                while [ $(( SECONDS - START_TIME )) -lt 10 ]; do
                    ping -c 1 -W 1 192.168.2.1 >/dev/null 2>&1 && { PRIV_SUCCESS=true; break 2; }
                    sleep 0.5
                done
                kill $NM_PID 2>/dev/null
            done
            [ "$PRIV_SUCCESS" = false ] && exit 1
            echo "100"; echo "# Active Tunnel!"; sleep 1
        ) | zenity --progress --title="Double VPN Sequence" --width=450 --auto-close --auto-kill

        if [ ${PIPESTATUS[0]} -ne 0 ]; then
            nmcli connection down "$PRIV" &>/dev/null; nmcli connection down "$BRIDGE" &>/dev/null; continue
        fi
        TARGET="$USER@$IP"; OPTS=(-o "KexAlgorithms=curve25519-sha256")
        
    elif [[ "$CHOICE" == D* ]]; then
        [ "$CHOICE" == "D1" ] && { USER="$D1_USER"; HOST="$D1_HOST"; PORT="$D1_PORT"; } || { USER="$D2_USER"; HOST="$D2_HOST"; PORT="$D2_PORT"; }
        TARGET="$USER@$HOST"; OPTS=(-p "$PORT" -o "StrictHostKeyChecking=no")
    fi

    # Application Launcher[cite: 4]
    case "$APP_CHOICE" in
        "Terminal")
            [ "$USE_SSHPASS" = true ] && konsole --nofork -e bash -c "sshpass -P 'pass' -p '$GLOBAL_SSH_PASS' ssh ${OPTS[*]} '$TARGET'" \
                                      || konsole --nofork -e ssh "${OPTS[@]}" "$TARGET" ;;
        "Firefox") "${WP_PREFIX[@]}" "${OPTS[@]}" -R /tmp/pulse.sock:/run/user/1000/pulse/native "$TARGET" env PULSE_SERVER=unix:/tmp/pulse.sock firefox ;;
        "Dolphin") "${WP_PREFIX[@]}" "${OPTS[@]}" "$TARGET" dolphin ;;
        "KWrite")  "${WP_PREFIX[@]}" "${OPTS[@]}" "$TARGET" kwrite ;;
    esac

    [[ "$CHOICE" == V* ]] && { nmcli connection down "$PRIV" &>/dev/null; nmcli connection down "$BRIDGE" &>/dev/null; }
done
