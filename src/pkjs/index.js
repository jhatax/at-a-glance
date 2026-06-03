var Clay = require("pebble-clay");
var clayConfig = require("./config.json");
var clay = new Clay(clayConfig);

const WEATHER_INTERVAL_MS = 30 * 60 * 1000;
const DEFAULT_LAT = 37.85626;
const DEFAULT_LON = -122.21383;

function sendTemperature(celsius) {
  Pebble.sendAppMessage({ TEMPERATURE: Math.round(celsius * 10) });
}

function fetchWeather(lat, lon) {
  var url =
    "https://api.open-meteo.com/v1/forecast?latitude=" +
    lat +
    "&longitude=" +
    lon +
    "&current=temperature_2m&temperature_unit=celsius";

  var xhr = new XMLHttpRequest();
  xhr.open("GET", url, true);
  xhr.onload = function () {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var data = JSON.parse(xhr.responseText);
        if (data.current && typeof data.current.temperature_2m === "number") {
          sendTemperature(data.current.temperature_2m);
        }
      } catch (e) {
        console.log("AtAGlance: Weather parse error");
      }
    }
  };
  xhr.onerror = function () {
    console.log("AtAGlance: Weather request failed");
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
