// Clay stuff
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var messageKeys = require('message_keys');

const STEPS_GOAL_MIN = 4000;
const STEPS_GOAL_DEFAULT = 10000;
const STEPS_GOAL_MAX = 32000;

const WEATHER_UPDATE_MINUTES_DEFAULT = 15;
const UPDATE_INTERVAL_MAX = WEATHER_UPDATE_MINUTES_DEFAULT << 4; // (15 * 16: 4-hours max)
const MAX_LOCATION_STRING_LENGTH = 15;
var s_updateIntervalHandle = null;
var s_currentUpdateInterval = WEATHER_UPDATE_MINUTES_DEFAULT;
var s_startingUpdateInterval = WEATHER_UPDATE_MINUTES_DEFAULT;

function clampStepsGoal(goal) {
  if (goal < STEPS_GOAL_MIN) {
    return STEPS_GOAL_MIN;
  }
  if (goal > STEPS_GOAL_MAX) {
    return STEPS_GOAL_MAX;
  }
  return goal;
}

function parseStepsGoal(rawSettings) {
  var goal = STEPS_GOAL_DEFAULT;
  if (rawSettings) {
    var presetGoal = parseInt(rawSettings.STEPS_GOAL_PRESET, 10);
    var customGoal = parseInt(rawSettings.STEPS_GOAL_CUSTOM, 10);

    if (!isNaN(customGoal)) {
      goal = clampStepsGoal(customGoal);
    } else if (!isNaN(presetGoal)) {
      goal = clampStepsGoal(presetGoal);
    }
  }

  return goal;
}

function updateWeatherInterval(newInterval) {
  s_currentUpdateInterval = newInterval;
  // Weather Update Interval is going to backoff on timeout,
  // so save user-pref to reset interval to user's preference.
  s_startingUpdateInterval = newInterval;
}

// Return the default value if setting is invalid or irretrievable
function parseWeatherUpdateMinutes(rawSettings) {
  var toreturn = rawSettings
    ? parseInt(rawSettings.WEATHER_UPDATE_MINUTES, 10)
    : WEATHER_UPDATE_MINUTES_DEFAULT;
  return isNaN(toreturn) ? WEATHER_UPDATE_MINUTES_DEFAULT : toreturn;
}

function applyWeatherUpdateMinutes(minutes) {
  var shouldUpdateRefreshSchedule = false;
  if (isNaN(minutes)) {
    // Reset only if needed.
    if (s_currentUpdateInterval != WEATHER_UPDATE_MINUTES_DEFAULT) {
      shouldUpdateRefreshSchedule = true;
      updateWeatherInterval(WEATHER_UPDATE_MINUTES_DEFAULT);
    }
  } else {
    // If the current s_weatherUpdateInterval is not the same as the input
    // to which the interval is going to be set, let's update the refresh schedule.
    if (s_currentUpdateInterval != minutes) {
      // Is the input from user within the accepted range?
      if (minutes > 0 && minutes <= UPDATE_INTERVAL_MAX) {
        shouldUpdateRefreshSchedule = true;
        updateWeatherInterval(minutes);
      }
    }
  }

  updateWeatherAndLocation();
  if (shouldUpdateRefreshSchedule) {
    scheduleUpdates();
  }
}

var clay = new Clay(clayConfig);

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('appmessage', function (e) {
  if (!e || !e.payload) {
    return;
  }

  if (typeof e.payload[messageKeys.WEATHER_UPDATE_MINUTES] === 'number') {
    applyWeatherUpdateMinutes(e.payload[messageKeys.WEATHER_UPDATE_MINUTES]);
  }
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    return;
  }

  var dict = clay.getSettings(e.response);
  var rawSettings = clay.getSettings(e.response, false);
  dict[messageKeys.STEPS_GOAL] = parseStepsGoal(rawSettings);
  delete dict[messageKeys.STEPS_GOAL_PRESET];
  delete dict[messageKeys.STEPS_GOAL_CUSTOM];
  applyWeatherUpdateMinutes(parseWeatherUpdateMinutes(rawSettings));
  Pebble.sendAppMessage(
    dict,
    function () {
      console.log('Sent config data to Pebble');
    },
    function (err) {
      console.log('Failed to send config data!');
    }
  );
});

// Weather stuff
// Must match WATCHFACE_WEATHER_TEMP_UNAVAILABLE in src/modules/watchface.c.
var WEATHER_TEMP_INVALID = -32768;
// Must match WATCHFACE_WEATHER_CONDITION_UNKNOWN in src/modules/watchface.c.
var WEATHER_CONDITION_UNKNOWN = -1;
// OAK is the product fallback location for this watchface.
var OAK_WEATHER_LATITUDE = 37.85626;
var OAK_WEATHER_LONGITUDE = -122.21383;

// Define custom function that manipulates the DOM elements inside index.js
function sendWeather(celsius, weatherCode, isDay) {
  Pebble.sendAppMessage(
    {
      TEMPERATURE: Math.round(celsius * 10),
      WEATHER_CONDITION: weatherCode,
      IS_DAY: isDay ? 1 : 0,
    },
    null,
    function (e) {
      console.log('AtAGlance: Weather send failed: ' + e.error);
    }
  );
}

function sendWeatherUnavailable(reason) {
  console.log('AtAGlance: Weather unavailable: ' + reason);
  Pebble.sendAppMessage(
    {
      TEMPERATURE: WEATHER_TEMP_INVALID,
      WEATHER_CONDITION: WEATHER_CONDITION_UNKNOWN,
      IS_DAY: 0,
    },
    null,
    function (e) {
      console.log('AtAGlance: Weather unavailable send failed: ' + e.error);
    }
  );
}

function updateIntervalMs() {
  return s_currentUpdateInterval * 60 * 1000;
}

function sendLocationToWatch(location) {
  Pebble.sendAppMessage({ MAYBE_CURRENT_LOCATION: location }, null, function (e) {
    console.log('AtAGlance: Location update failed: ' + e.error);
  });
}

function fetchLocation(lat, lon) {
  var url = 'https://nominatim.openstreetmap.org/reverse?format=json&lat=' + lat + '&lon=' + lon;

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url);
  // Nominatim requires a valid, unique User-Agent header
  xhr.setRequestHeader('User-Agent', 'PebbleWatchFace-AtAGlance');
  xhr.timeout = updateIntervalMs();

  xhr.ontimeout = function () {
    console.log('Network request timed out.');
    Pebble.sendAppMessage({ MAYBE_CURRENT_LOCATION: '' });
  };

  xhr.onerror = function () {
    console.log('Network request encountered an error.');
    Pebble.sendAppMessage({ MAYBE_CURRENT_LOCATION: '' });
  };

  xhr.onload = function () {
    var city = '';
    try {
      if (xhr.readyState === 4 && xhr.status === 200) {
        var response = JSON.parse(xhr.responseText);

        city = (response.address.city || response.address.town || response.address.village || 'GPS')
          .slice(0, MAX_LOCATION_STRING_LENGTH)
          .toUpperCase();
      }
    } catch (e) {
      city = '';
    } finally {
      sendLocationToWatch(city);
    }
  };
  xhr.send();
}

function fetchWeather(lat, lon) {
  var url =
    'https://api.open-meteo.com/v1/forecast?latitude=' +
    lat +
    '&longitude=' +
    lon +
    '&current=temperature_2m,weather_code,is_day&temperature_unit=celsius';

  var xhr = new XMLHttpRequest();
  xhr.open('GET', url, true);
  xhr.timeout = updateIntervalMs();
  xhr.onload = function () {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (
          data.current &&
          typeof data.current.temperature_2m === 'number' &&
          typeof data.current.weather_code === 'number' &&
          typeof data.current.is_day === 'number'
        ) {
          sendWeather(
            data.current.temperature_2m,
            data.current.weather_code,
            data.current.is_day === 1
          );
        } else {
          sendWeatherUnavailable('malformed response');
        }
      } catch (e) {
        sendWeatherUnavailable('parse error');
      }
    } else if (xhr.readyState === 4) {
      sendWeatherUnavailable('request status ' + xhr.status);
    }
  };
  xhr.onerror = function () {
    sendWeatherUnavailable('request failed');
  };
  xhr.ontimeout = function () {
    // exponentially back-off
    // if the value is the max, you need to back-off to the original value
    s_currentUpdateInterval =
      s_currentUpdateInterval < UPDATE_INTERVAL_MAX
        ? s_currentUpdateInterval * 2
        : s_startingUpdateInterval;
    if (s_currentUpdateInterval >= UPDATE_INTERVAL_MAX) {
      // Clamp the update interval to UPDATE_INTERVAL_MAX
      s_currentUpdateInterval = Math.min(s_currentUpdateInterval, UPDATE_INTERVAL_MAX);
      sendWeatherUnavailable('request timed out');
    }
    scheduleUpdates();
  };
  xhr.send(null);
}

function updateWeatherAndLocation() {
  if (typeof navigator !== 'undefined' && navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        fetchWeather(pos.coords.latitude, pos.coords.longitude);
        fetchLocation(pos.coords.latitude, pos.coords.longitude);
      },
      function () {
        fetchWeather(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
        fetchLocation(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
      },
      { timeout: updateIntervalMs(), maximumAge: updateIntervalMs() }
    );
  } else {
    fetchWeather(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
    fetchLocation(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
  }
}

function scheduleUpdates() {
  if (s_updateIntervalHandle !== null) {
    clearInterval(s_updateIntervalHandle);
  }

  s_updateIntervalHandle = setInterval(updateWeatherAndLocation, updateIntervalMs());
}

Pebble.addEventListener('ready', function () {
  updateWeatherAndLocation();
  scheduleUpdates();
});
