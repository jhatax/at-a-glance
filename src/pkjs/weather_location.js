const REQUEST_TIMEOUT = 2 * 60 * 1000;
const GEO_REQUEST_AGE = 30 * 60 * 1000;
const MAX_LOCATION_STRING_LENGTH = 15;
const WEATHER_UPDATE_MINUTES_DEFAULT = 15;

function createRefreshController(dependencies) {
  var currentUpdateInterval = -1;
  var updateIntervalHandle = null;
  var lastKnownLocation = null;

  if (
    !dependencies ||
    typeof dependencies.sendWeatherMessage !== 'function' ||
    typeof dependencies.sendLocationMessage !== 'function'
  ) {
    throw new Error('Refresh controller requires weather and location senders');
  }

  function isCurrentLocation(location) {
    return location === lastKnownLocation;
  }

  function isSameLocation(pos) {
    return (
      lastKnownLocation &&
      lastKnownLocation.latitude === pos.coords.latitude &&
      lastKnownLocation.longitude === pos.coords.longitude
    );
  }

  function sendWeather(celsius, weatherCode, isDay, location) {
    if (!isCurrentLocation(location)) {
      return;
    }

    dependencies.sendWeatherMessage(
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

  function sendLocationToWatch(location) {
    dependencies.sendLocationMessage({ MAYBE_CURRENT_LOCATION: location }, null, function (e) {
      console.log('AtAGlance: Location update failed: ' + e.error);
    });
  }

  function fetchLocation(location) {
    var url =
      'https://nominatim.openstreetmap.org/reverse?format=json&lat=' +
      location.latitude +
      '&lon=' +
      location.longitude;

    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.setRequestHeader('User-Agent', 'PebbleWatchFace-AtAGlance');
    xhr.timeout = REQUEST_TIMEOUT;
    xhr.onload = function () {
      if (!isCurrentLocation(location) || xhr.readyState !== 4 || xhr.status !== 200) {
        return;
      }

      try {
        var response = JSON.parse(xhr.responseText);
        var location = (
          response.address.neighborhood ||
          response.address.suburb ||
          response.address.borough ||
          response.address.village ||
          response.address.town ||
          response.address.city ||
          response.address.county ||
          'GPS'
        )
          .slice(0, MAX_LOCATION_STRING_LENGTH)
          .toUpperCase();
        sendLocationToWatch(location);
      } catch (e) {
        console.log('Location fetch error: ' + JSON.stringify(e));
      }
    };
    xhr.send();
  }

  function fetchWeather(location) {
    var url =
      'https://api.open-meteo.com/v1/forecast?latitude=' +
      location.latitude +
      '&longitude=' +
      location.longitude +
      '&current=temperature_2m,weather_code,is_day&temperature_unit=celsius';

    var xhr = new XMLHttpRequest();
    xhr.open('GET', url, true);
    xhr.timeout = REQUEST_TIMEOUT;
    xhr.onload = function () {
      if (!isCurrentLocation(location) || xhr.readyState !== 4 || xhr.status !== 200) {
        return;
      }

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
            data.current.is_day === 1,
            location
          );
        }
      } catch (e) {
        console.log('Weather fetch error: ' + JSON.stringify(e));
      }
    };
    xhr.send();
  }

  function refreshClimateAndLocation(location) {
    fetchWeather(location);
    fetchLocation(location);
  }

  function requestLocation() {
    if (typeof navigator === 'undefined' || !navigator.geolocation) {
      return;
    }

    // Request ordering; discard a response that's older than the current request
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        if (isSameLocation(pos)) {
          return;
        }

        lastKnownLocation = {
          latitude: pos.coords.latitude,
          longitude: pos.coords.longitude,
        };
        refreshClimateAndLocation(lastKnownLocation);
      },
      function (e) {
        console.log('Geolocation error: ' + JSON.stringify(e));
      },
      { timeout: REQUEST_TIMEOUT, maximumAge: GEO_REQUEST_AGE }
    );
  }

  function refreshNow() {
    if (lastKnownLocation) {
      refreshClimateAndLocation(lastKnownLocation);
    }
    requestLocation();
  }

  function scheduleUpdates() {
    if (updateIntervalHandle !== null) {
      clearInterval(updateIntervalHandle);
      updateIntervalHandle = null;
    }
    updateIntervalHandle = setInterval(refreshNow, currentUpdateInterval * 60 * 1000);
  }
  function setIntervalMinutes(minutes) {
    var isValid = typeof minutes === 'number' && minutes > 0;
    var nextInterval = isValid ? minutes : WEATHER_UPDATE_MINUTES_DEFAULT;
    var changed = currentUpdateInterval !== nextInterval;
    if (changed || updateIntervalHandle === null) {
      currentUpdateInterval = nextInterval;
      refreshNow();
      scheduleUpdates();
    }
  }

  return {
    start: function (minutes) {
      setIntervalMinutes(minutes);
    },
    setIntervalMinutes: setIntervalMinutes,
  };
}

module.exports = {
  createRefreshController: createRefreshController,
};
