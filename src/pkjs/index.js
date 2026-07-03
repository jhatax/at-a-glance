// Clay stuff
var Clay = require("@rebble/clay");
var clayConfig = require("./config.json");
var messageKeys = require("message_keys");

var STEPS_GOAL_MIN = 4000;
var STEPS_GOAL_DEFAULT = 10000;
var STEPS_GOAL_MAX = 32000;

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
    var customGoalText = rawSettings.STEPS_GOAL_CUSTOM;
    var customGoal = parseInt(customGoalText, 10);

    if (!isNaN(customGoal)) {
      goal = clampStepsGoal(customGoal);
    } else if (!isNaN(presetGoal)) {
      goal = clampStepsGoal(presetGoal);
    }
  }

  return goal;
}

var clay = new Clay(clayConfig);

Pebble.addEventListener("showConfiguration", function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener("webviewclosed", function(e) {
  if (!e || !e.response) {
    return;
  }

  var dict = clay.getSettings(e.response);
  var rawSettings = clay.getSettings(e.response, false);
  var stepsGoal = parseStepsGoal(rawSettings);

  delete dict[messageKeys.STEPS_GOAL_PRESET];
  delete dict[messageKeys.STEPS_GOAL_CUSTOM];
  dict[messageKeys.STEPS_GOAL] = stepsGoal;

  Pebble.sendAppMessage(dict, function() {
    console.log("Sent config data to Pebble");
  }, function(err) {
    console.log("Failed to send config data!");
    console.log(JSON.stringify(err));
  });
});

// Weather stuff
var WEATHER_INTERVAL_MS = 15 * 60 * 1000;
// Must match WATCHFACE_WEATHER_TEMP_UNAVAILABLE in src/modules/watchface.c.
var WEATHER_TEMP_INVALID = -32768;
// Must match WATCHFACE_WEATHER_CONDITION_UNKNOWN in src/modules/watchface.c.
var WEATHER_CONDITION_UNKNOWN = -1;
// OAK is the product fallback location for this watchface.
var OAK_WEATHER_LATITUDE = 37.85626;
var OAK_WEATHER_LONGITUDE = -122.21383;

var s_weatherRequestId = 0;
// Define custom function that manipulates the DOM elements inside index.js
function sendWeather(celsius, weatherCode, isDay) {
  Pebble.sendAppMessage(
    {
      TEMPERATURE: Math.round(celsius * 10),
      WEATHER_CONDITION: weatherCode,
      IS_DAY: isDay ? 1 : 0
    },
    null,
    function (e) {
      console.log("AtAGlance: Weather send failed: " + e.error);
    }
  );
}

function sendWeatherUnavailable(reason) {
  console.log("AtAGlance: Weather unavailable: " + reason);
  Pebble.sendAppMessage(
    {
      TEMPERATURE: WEATHER_TEMP_INVALID,
      WEATHER_CONDITION: WEATHER_CONDITION_UNKNOWN,
      IS_DAY: 0
    },
    null,
    function (e) {
      console.log(
        "AtAGlance: Weather unavailable send failed: " + e.error);
    }
  );
}

function fetchWeather(lat, lon, requestId) {
  var url =
    "https://api.open-meteo.com/v1/forecast?latitude=" +
    lat +
    "&longitude=" +
    lon +
    "&current=temperature_2m,weather_code,is_day&temperature_unit=celsius";

  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.timeout = WEATHER_INTERVAL_MS;
  xhr.onload = function () {
    if (requestId !== s_weatherRequestId) {
      return;
    }

    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (
          data.current &&
          typeof data.current.temperature_2m === "number" &&
          typeof data.current.weather_code === "number" &&
          typeof data.current.is_day === "number"
        ) {
          sendWeather(data.current.temperature_2m,
                      data.current.weather_code,
                      data.current.is_day === 1);
        } else {
          sendWeatherUnavailable("malformed response");
        }
      } catch (e) {
        sendWeatherUnavailable("parse error");
      }
    } else if (xhr.readyState === 4) {
      sendWeatherUnavailable("request status " + xhr.status);
    }
  };
  xhr.onerror = function () {
    if (requestId !== s_weatherRequestId) {
      return;
    }

    sendWeatherUnavailable("request failed");
  };
  xhr.ontimeout = function () {
    if (requestId !== s_weatherRequestId) {
      return;
    }

    sendWeatherUnavailable("request timed out");
  };
  xhr.send(null);
}

function updateWeather() {
  var requestId = ++s_weatherRequestId;

  if (typeof navigator !== "undefined" && navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        if (requestId !== s_weatherRequestId) {
          return;
        }

        fetchWeather(pos.coords.latitude, pos.coords.longitude, requestId);
      },
      function () {
        if (requestId !== s_weatherRequestId) {
          return;
        }

        fetchWeather(
            OAK_WEATHER_LATITUDE,
            OAK_WEATHER_LONGITUDE,
            requestId);
      },
      { timeout: WEATHER_INTERVAL_MS, maximumAge: WEATHER_INTERVAL_MS }
    );
  } else {
    fetchWeather(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE, requestId);
  }
}

Pebble.addEventListener("ready", function () {
  updateWeather();
  setInterval(updateWeather, WEATHER_INTERVAL_MS);
});
