#!/usr/bin/env zsh

# CONFIG Constants
typeset -gr -a QA_DEFAULT_EMULATORS=(gabbro emery chalk flint)
typeset -gr -a QA_SUPPORTED_EMULATORS=(aplite chalk diorite emery flint gabbro)
typeset -gr -a QA_SUPPORTED_TESTS=(battery health smoke weather)

# DISPLAYS
typeset -gr -a QA_DISPLAY_MODES=(0 1 2 3)

# WEATHER
typeset -gr -a QA_WEATHER_CODES=(0 1 2 3 45 51 55 57 61 65 67 71 80 82 85 86 95 -1)
typeset -gr -a QA_SMOKE_WEATHER_CODES=(0 2 3 57 65 67 86 95 -1)
typeset -gr -a QA_DAY_STATES=(0 1)
readonly QA_WEATHER_TEST_TEMPERATURE=539

# BATTERY
typeset -gr -a QA_BATTERY_LEVELS=(1 10 20 25 50 60 70 80 90 100)
typeset -gr -a QA_BATTERY_CHARGING_STATES=(1 0)

# HEALTH
typeset -gr -a QA_HEALTH_BPM_VALUES=(99 101 121)
typeset -gr -a QA_HEALTH_STEPS_VALUES=(1000 8000 11000)