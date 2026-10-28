const REQUEST_TIMEOUT = 2 * 60 * 1000;
const GEO_REQUEST_AGE = 30 * 60 * 1000;
const MAX_LOCATION_STRING_LENGTH = 15;
const WEATHER_UPDATE_MINUTES_DEFAULT = 15;

function createRefreshController(dependencies) {
  var currentUpdateInterval = -1;
  var updateIntervalHandle = null;
  var lastKnownLocation = null;

  if (!dependencies || typeof dependencies.sendClimateMessage !== 'function') {
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

  function sendMessage(payload) {
    if (!payload) {
      return;
    }
    dependencies.sendClimateMessage(payload, null, function (e) {
      console.log(
        'AtAGlance: Climate, Location Payload send failed: (' + e.code + '): ' + e.message
      );
    });
  }

  function createXHRForPKJS(type, url, reject) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', url);
    xhr.setRequestHeader('User-Agent', 'PebbleWatchFace-AtAGlance');
    xhr.timeout = REQUEST_TIMEOUT;
    xhr.onerror = function () {
      reject(type + ' network request failed entirely.');
    };

    xhr.ontimeout = function () {
      reject(type + ' request timed out.');
    };

    xhr.onloadend = function () {
      if (xhr.status === 0) {
        reject(type + ' request ended without a response.');
      }
    };

    return xhr;
  }
  function fetchLocation(location) {
    return new Promise(function (resolve, reject) {
      var url =
        'https://nominatim.openstreetmap.org/reverse?format=jsonv2&addressdetails=1&lat=' +
        location.latitude +
        '&lon=' +
        location.longitude;

      var xhr = createXHRForPKJS('Location', url, reject);
      xhr.onload = function () {
        if (!isCurrentLocation(location)) {
          reject('Out of order fetch; location has changed');
          return;
        }

        if (xhr.status !== 200) {
          reject('HTTP request failed: status ' + xhr.status);
          return;
        }

        try {
          var address = JSON.parse(xhr.responseText).address || {};
          resolve({
            MAYBE_CURRENT_LOCATION: (
              address.neighbourhood ||
              address.suburb ||
              address.village ||
              address.borough ||
              address.town ||
              address.city ||
              address.county ||
              address.state ||
              ''
            )
              .slice(0, MAX_LOCATION_STRING_LENGTH)
              .toUpperCase(),
          });
        } catch (e) {
          reject('Location fetch error: (' + e.code + '): ' + e.message);
        }
      };
      xhr.send();
    });
  }

  function fetchWeather(location) {
    return new Promise(function (resolve, reject) {
      var url =
        'https://api.open-meteo.com/v1/forecast?latitude=' +
        location.latitude +
        '&longitude=' +
        location.longitude +
        '&current=temperature_2m,weather_code,is_day&temperature_unit=celsius';

      var xhr = createXHRForPKJS('Weather', url, reject);
      xhr.onload = function () {
        if (!isCurrentLocation(location)) {
          reject('Out of order fetch; location has changed');
          return;
        }

        if (xhr.status !== 200) {
          reject('HTTP request failed: status ' + xhr.status);
          return;
        }

        try {
          var data = JSON.parse(xhr.responseText);
          var info = data.current;
          if (
            info &&
            typeof info.temperature_2m === 'number' &&
            typeof info.weather_code === 'number' &&
            typeof info.is_day === 'number'
          ) {
            resolve({
              TEMPERATURE: Math.round(info.temperature_2m * 10),
              WEATHER_CONDITION: info.weather_code,
              IS_DAY: info.is_day,
            });
          } else {
            reject('Weather fetch retrieved invalid values');
            return;
          }
        } catch (e) {
          reject('Weather fetch error: (' + e.code + '): ' + e.message);
        }
      };
      xhr.send();
    });
  }

  function clearWeatherAndLocation() {
    lastKnownLocation = null;
    sendMessage({
      TEMPERATURE: 0,
      WEATHER_CONDITION: -1,
      IS_DAY: -1,
      MAYBE_CURRENT_LOCATION: '',
    });
  }

  function refreshClimateAndLocation(location) {
    Promise.all([fetchWeather(location), fetchLocation(location)])
      .then(function (results) {
        var weather = results[0];
        var locationData = results[1];
        sendMessage({
          TEMPERATURE: weather.TEMPERATURE,
          WEATHER_CONDITION: weather.WEATHER_CONDITION,
          IS_DAY: weather.IS_DAY,
          MAYBE_CURRENT_LOCATION: locationData.MAYBE_CURRENT_LOCATION,
        });
      })
      .catch(function (error) {
        if (!isCurrentLocation(location)) {
          return;
        }

        console.log('Failed to gather climate & location data collectively.\n' + error);
        clearWeatherAndLocation();
      });
  }

  function requestLocation() {
    if (typeof navigator === 'undefined' || !navigator.geolocation) {
      clearWeatherAndLocation();
      return;
    }

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
        console.log('Geolocation error: (' + e.code + '): ' + e.message);
        clearWeatherAndLocation();
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
