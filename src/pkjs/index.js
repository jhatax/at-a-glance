var Clay = require("@rebble/clay");
var clayConfig = require("./config.json");
var clay = new Clay(clayConfig);

const WEATHER_INTERVAL_MS = 30 * 60 * 1000;
// Must match WEATHER_TEMP_INVALID in src/modules/climate.h.
const WEATHER_TEMP_INVALID = -32768;
// Must match WEATHER_CONDITION_UNKNOWN in src/modules/climate.h.
const WEATHER_CONDITION_UNKNOWN = -1;
// OAK is the product fallback location for this watchface.
const OAK_WEATHER_LATITUDE = 37.85626;
const OAK_WEATHER_LONGITUDE = -122.21383;

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

function fetchWeather(lat, lon) {
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
    sendWeatherUnavailable("request failed");
  };
  xhr.ontimeout = function () {
    sendWeatherUnavailable("request timed out");
  };
  xhr.send(null);
}

function updateWeather() {
  if (typeof navigator !== "undefined" && navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        fetchWeather(pos.coords.latitude, pos.coords.longitude);
      },
      function () {
        fetchWeather(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
      },
      { timeout: WEATHER_INTERVAL_MS, maximumAge: WEATHER_INTERVAL_MS }
    );
  } else {
    fetchWeather(OAK_WEATHER_LATITUDE, OAK_WEATHER_LONGITUDE);
  }
}

Pebble.addEventListener("ready", function () {
  updateWeather();
  setInterval(updateWeather, WEATHER_INTERVAL_MS);
});
