var Clay = require("@rebble/clay");
var clayConfig = require("./config.json");
var clay = new Clay(clayConfig);

const WEATHER_INTERVAL_MS = 30 * 60 * 1000;
// Must match WEATHER_TEMP_INVALID in src/modules/weather.h.
const WEATHER_TEMP_INVALID = -32768;
// Must match WEATHER_CONDITION_UNKNOWN in src/modules/weather.h.
const WEATHER_CONDITION_UNKNOWN = -1;
const DEFAULT_LAT = 37.85626;
const DEFAULT_LON = -122.21383;

function sendWeather(celsius, weatherCode) {
  Pebble.sendAppMessage({
    TEMPERATURE: Math.round(celsius * 10),
    WEATHER_CONDITION: weatherCode
  });
}

function sendWeatherUnavailable(reason) {
  console.log("AtAGlance: Weather unavailable: " + reason);
  Pebble.sendAppMessage({
    TEMPERATURE: WEATHER_TEMP_INVALID,
    WEATHER_CONDITION: WEATHER_CONDITION_UNKNOWN
  });
}

function fetchWeather(lat, lon) {
  var url =
    "https://api.open-meteo.com/v1/forecast?latitude=" +
    lat +
    "&longitude=" +
    lon +
    "&current=temperature_2m,weather_code&temperature_unit=celsius";

  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.onload = function () {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (
          data.current &&
          typeof data.current.temperature_2m === "number" &&
          typeof data.current.weather_code === "number"
        ) {
          sendWeather(data.current.temperature_2m,
                      data.current.weather_code);
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
  xhr.send(null);
}

function updateWeather() {
  if (typeof navigator !== "undefined" && navigator.geolocation) {
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        fetchWeather(pos.coords.latitude, pos.coords.longitude);
      },
      function () {
        fetchWeather(DEFAULT_LAT, DEFAULT_LON);
      },
      { timeout: WEATHER_INTERVAL_MS, maximumAge: WEATHER_INTERVAL_MS }
    );
  } else {
    fetchWeather(DEFAULT_LAT, DEFAULT_LON);
  }
}

Pebble.addEventListener("ready", function () {
  updateWeather();
  setInterval(updateWeather, WEATHER_INTERVAL_MS);
});
