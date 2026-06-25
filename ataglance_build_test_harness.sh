#!/usr/bin/env zsh

# AtAGlance build/test harness.
#
# Flow:
#   1. Optional build or clean build.
#   2. Optional emulator install for the selected emulator list.
#   3. Optional physical-watch install through the iOS/Android Pebble app
#      Developer Connection server.
#   4. Optional emulator automation for weather AppMessage payloads and battery
#      emulator state changes.
#
# Phone installs require the Server IP shown by the Pebble/RePebble mobile app.
# A 169.254.x.x address is link-local and is not usable for Mac-to-phone
# Developer Connection over the normal LAN.
set -e

local emulators=(gabbro emery chalk flint)
# local codes=(0 1 2 3 45 51 55 57 67 86 61 65 67 71 80 82 85 95 -1)
local codes=(0 1 2 3)
local modes=(0 1 2 3)
local isday=(0 1)
local tests=(weather battery)


# --- 1. Default Configuration State ---
local BUILD_CLEAN=false
local BUILD=false
local INSTALL=false
local WIPE=false
local OBLITERATE=false
local RUN_AUTOMATION=false
local PHONE_IP=""
local PHONE_INSTALL=false

# --- 2. Multiple Argument Parsing Loop ---
# Loop as long as there is an argument ($1) available
while [[ $# -gt 0 ]]; do
    case "$1" in
        -bc|--build-clean|--clean)
            BUILD_CLEAN=true
            shift # Move to the next argument
            ;;
        -b|--build)
            BUILD=true
            shift # Move to the next argument
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -p|--phone)
            if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
                echo "❌ Error: -p/--phone requires a Pebble app Server IP." >&2
                exit 1
            fi

            PHONE_IP="$2"
            PHONE_INSTALL=true
            shift 2
            ;;
        -e|--emulators)
            # Ensure a value actually follows the flag
            if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
                echo "❌ Error: -e/--emulators requires a comma-separated list of values." >&2
                exit 1
            fi

            # Split the comma-separated string ($2) into a Zsh array
            # s flag splits by the character inside quotes
            emulators=(${(s:,:)2})

            shift 2 # Move past both the flag (-e) and its value (gabbro,emery)
            ;;
        -w|--wipe)
            WIPE=true
            shift
            ;;
        -t|--test)
            RUN_AUTOMATION=true
            # Figure out what needs to be tested
            if [[ -z "${2:-}" || "${2:0:1}" == "-" ]]; then
                echo "Running all tests...\n"
                TEST_ALL=true
                shift
            else
                # Split the comma-separated string ($2) into a Zsh array
                # s flag splits by the character inside quotes
                tests=(${(s:,:)2})
                shift 2 # Move past both the flag (-t) and its value
            fi
	    ;;
        -n|--nuclear)
            OBLITERATE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  -b, --build             Build the project before sending messages"
            echo "  -bc, --build-clean      Clean & Build the project before sending messages"
            echo "  -i, --install           Install on emulators specified with -e. Default is to install / run tests on emery, flint, chalk, gabbro."
            echo "  -p, --phone IP          Install on a paired watch through the mobile app Developer Connection Server IP."
            echo "  -e, --emulators         Use with -i or -t to specify emulators using commas (emery, flint, chalk, gabbro, aplite, diorite)"
            echo "  -t, --test              Use with -e to specify tests (using commas) to run on emulators. Default is to run {weather,battery}."
            echo "  -w, --wipe              Wipe emulator data"
            echo "  -n, --nuclear           Terminate all emulators, wipe, build-clean"
            echo "  -h, --help              Show this help menu"
            exit 0
            ;;
        *)
            echo "❌ Error: Unknown argument '$1'" >&2
            exit 1
            ;;
    esac
done

# --- 3. Conditional Actions Based on Flags ---
if [[ "$BUILD" == true ]]; then
    echo "🔨 Building the Pebble project...\n"
    pebble build
    echo "✅ Build completed successfully.\n"
fi

if [[ "$BUILD_CLEAN" == true ]]; then
    echo "🔨 Building the Pebble project...\n"
    pebble clean && pebble build
    echo "✅ Clean Build completed successfully.\n"
fi

if [[ "$WIPE" == true ]]; then
    echo "Wiping emulator cache\n"
    pebble wipe
    echo "✅ Wipe success.\n"
fi

if [[ "$INSTALL" == true ]]; then
    echo "Launching selected emulators with the built binary...\n"
    # Launch emulators
    for emu in $emulators; do
	sleep 4
        pebble install --emulator "$emu"
    done
fi

if [[ "$PHONE_INSTALL" == true ]]; then
    if [[ "$PHONE_IP" == 169.254.* ]]; then
        echo "❌ Error: $PHONE_IP is a link-local address, not a usable LAN Server IP." >&2
        echo "   Reconnect Wi-Fi, check iOS Local Network permission, and restart Developer Connection." >&2
        exit 1
    fi

    echo "Installing through Pebble app Developer Connection at $PHONE_IP...\n"
    pebble install --phone "$PHONE_IP"
fi

if [[ "$OBLITERATE" == true ]]; then
    echo "⚠️  Wiping the slate clean, terminating all emulators, running a clean build...\n"
    # Wipe the slate clean and rebuild
    pebble kill --force
    pebble wipe
    pebble clean
    pebble build

fi

if [[ "$RUN_AUTOMATION" == true ]]; then
    for t in $tests; do
        case "$t" in
        weather)
            for emu in $emulators; do
                pebble install --emulator "$emu"
                sleep 2
            done
            sleep 5
            echo "Starting automation to view weather glyphs using send-app-message...\n"
            # Outer loop: Light and dark modes
            for m in $modes; do
                for d in $isday; do # Night and day
                    for c in $codes; do # Each code
                        # Inner loop: Send code to all active emulators
                        for emu in $emulators; do # Each emulator
                            pebble send-app-message \
                                --emulator "$emu" \
                                --int 10002=539 10003="$c" 10004="$d" 10006="$m" 2>/dev/null
                            sleep 1
                        done
                    done
                    echo "Switching night to day"
                    sleep 1
                done
                echo "Switching modes"
            done
            echo "All emulator message sequences completed."
            ;;

        battery)
            echo "Starting battery tests: 1 10 20 25 35 49 50 55 60 70 75 80 90 95 100 with and without charging bolt"
            local levels=(1 10 20 25 35 49 50 55 60 70 75 80 90 95 100)
            local charging=(1 0)

            for emu in $emulators; do # Each emulator
                pebble install --emulator "$emu"
                echo "Testing on emulator $emu"
                for m in $modes; do # each display-mode
                    pebble send-app-message --emulator "$emu" --int 10006="$m" 2>/dev/null
                    sleep 3
                    for c in $charging; do # Not-charging, Charging
                        local suffix="${${c:#0}:+--charging}"
                        echo "Suffix is ${suffix:+"$suffix"}"
                        for l in $levels; do # Each level
                            echo "Setting battery level to $l on emulator $emu. Is charging: $c $suffix"
                            pebble emu-battery \
                                --emulator "$emu" \
                                --percent "$l" ${suffix:+"$suffix"}
                            sleep 3
                        done
                        echo "Switching to the next charging mode"
                    done
                    sleep 3
                done
            done
            ;;
        esac
    done
fi
