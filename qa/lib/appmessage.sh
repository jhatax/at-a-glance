#!/usr/bin/env zsh

local -r QA_MSG_TEMPERATURE=10002
local -r QA_MSG_WEATHER_CONDITION=10003
local -r QA_MSG_IS_DAY=10004
local -r QA_MSG_WEATHER_UPDATE_MINUTES=10005
local -r QA_MSG_DISPLAY_MODE=10006
local -r QA_MSG_ONESHOT_BPM=10020
local -r QA_MSG_ONESHOT_STEPS=10021

qa_send_display_mode() {
  local emulator="$1"
  local mode="$2"

  pebble send-app-message --emulator "${emulator}" --int \
    ${QA_MSG_DISPLAY_MODE}="${mode}" 2>/dev/null
}

qa_send_weather_message() {
  local emulator="$1"
  local temperature="$2"
  local weather_code="$3"
  local is_day="$4"
  local mode="$5"

  pebble send-app-message --emulator "${emulator}" --int \
    ${QA_MSG_TEMPERATURE}="${temperature}" \
    ${QA_MSG_WEATHER_CONDITION}="${weather_code}" \
    ${QA_MSG_IS_DAY}="${is_day}" \
    ${QA_MSG_DISPLAY_MODE}="${mode}" 2>/dev/null
}

qa_send_health_override() {
  local emulator="$1"
  local bpm="$2"
  local steps="$3"

  pebble send-app-message --emulator "${emulator}" --int \
    ${QA_MSG_ONESHOT_BPM}="${bpm}" \
    ${QA_MSG_ONESHOT_STEPS}="${steps}" 2>/dev/null
}
