const REQUEST_TIMEOUT = 15 * 1000;
const GEO_REQUEST_AGE = 10 * 60 * 1000;
const MAX_LOCATION_STRING_LENGTH = 15;

const location_request_options = {
  timeout: REQUEST_TIMEOUT,
  maximumAge: GEO_REQUEST_AGE,
  enableHighAccuracy: false,
};

function createRefreshController(updateClimateLocation, updateUser) {
  var waitInterval = -1;
  var waitTimeoutHandle = null;
  var lastKnownLocation = null;
  var lastKnownLookup = null;
  var lastKnownClimate = null;
  var requestChain = new ClimateLocationRequestQueue();
  var sendUserMsg = updateUser;

  if (!updateClimateLocation || typeof updateClimateLocation !== 'function') {
    throw new Error('Refresh controller requires weather and location sender to watchface');
  }

  function isSameLocation(coords) {
    // If the user hasn't moved more than 1km, treat it as the same
    const pos_tolerance = 0.02;

    if (lastKnownLocation) {
      var latDiff = Math.abs(lastKnownLocation.latitude - coords.latitude);
      var lonDiff = Math.abs(lastKnownLocation.longitude - coords.longitude);
    }

    return lastKnownLocation && latDiff < pos_tolerance && lonDiff < pos_tolerance;
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

  function reverseGPS(location) {
    return new Promise((resolve, reject) => {
      var url =
        'https://nominatim.openstreetmap.org/reverse?format=jsonv2&addressdetails=1&lat=' +
        location.latitude +
        '&lon=' +
        location.longitude;

      if (isSameLocation(location) && lastKnownLookup) {
        resolve(lastKnownLookup);
        return;
      }

      var xhr = createXHRForPKJS('Location', url, reject);
      xhr.onload = function () {
        if (xhr.status !== 200) {
          reject('HTTP request failed: status ' + xhr.status);
          return;
        }

        try {
          var address = JSON.parse(xhr.responseText).address;
          if (!address) {
            reject("Couldn't retrieve valid location during lookup");
            return;
          }
          if (!isSameLocation(location)) {
            // location has changed, need to reject this promise
            reject('Location changed during lookup');
            return;
          }
          resolve(
            (
              address.neighbourhood ||
              address.suburb ||
              address.village ||
              address.borough ||
              address.town ||
              address.city ||
              address.county ||
              address.state ||
              'GPS'
            )
              .slice(0, MAX_LOCATION_STRING_LENGTH)
              .toUpperCase()
          );
        } catch (e) {
          reject('Location fetch error: (' + e.code + '): ' + e.message);
        }
      };
      xhr.send();
    });
  }

  function fetchClimate(location) {
    return new Promise((resolve, reject) => {
      var url =
        'https://api.open-meteo.com/v1/forecast?latitude=' +
        location.latitude +
        '&longitude=' +
        location.longitude +
        '&current=temperature_2m,weather_code,is_day&temperature_unit=celsius';

      var xhr = createXHRForPKJS('Weather', url, reject);
      xhr.onload = function () {
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

  function clearClimateAndLocation() {
    lastKnownLocation = null;
    lastKnownLookup = null;
    lastKnownClimate = null;
    updateClimateLocation({
      TEMPERATURE: 0,
      WEATHER_CONDITION: -1,
      IS_DAY: 0,
      MAYBE_CURRENT_LOCATION: '',
    });
  }

  function oneShotCachedClimateAndLocation() {
    if (lastKnownClimate && lastKnownLookup) {
      updateClimateLocation({
        TEMPERATURE: lastKnownClimate.TEMPERATURE,
        WEATHER_CONDITION: lastKnownClimate.WEATHER_CONDITION,
        IS_DAY: lastKnownClimate.IS_DAY,
        MAYBE_CURRENT_LOCATION: lastKnownLookup,
      });
    }
  }

  function areEqualObjects(objA, objB) {
    if (!objA || !objB || typeof objA !== 'object' || typeof objB !== 'object') {
      return false;
    }
    if (objA === objB) return true;
    return (
      JSON.stringify(objA, Object.keys(objA).sort()) ===
      JSON.stringify(objB, Object.keys(objB).sort())
    );
  }

  function refreshClimateAndLocation(location) {
    return Promise.all([fetchClimate(location), reverseGPS(location)]).then((results) => {
      var cli = results[0];
      var loc = results[1];
      if (!cli || !loc) {
        clearClimateAndLocation();
        return;
      }
      var payload = {};
      var toSend = false;
      if (lastKnownLookup !== loc) {
        payload.MAYBE_CURRENT_LOCATION = loc;
        toSend = true;
      }

      if (!areEqualObjects(cli, lastKnownClimate)) {
        Object.keys(cli).forEach((key) => (payload[key] = cli[key]));
        toSend = true;
      }
      if (toSend) {
        // Use promise adoption to chain this promise's execution to the request for
        // location, climate, and lookup. This ensures no out of order execution.
        return new Promise((resolve) =>
          updateClimateLocation(
            payload,
            () => {
              lastKnownClimate = cli;
              lastKnownLookup = loc;
              resolve();
            },
            () => {
              resolve();
            }
          )
        );
      }
    });
  }

  function requestLocation() {
    return new Promise((resolve, reject) => {
      if (typeof navigator === 'undefined' || !navigator.geolocation) {
        reject("Couldn't retrieve geolocation");
        return;
      }

      navigator.geolocation.getCurrentPosition(
        (pos) => {
          if (isSameLocation(pos.coords)) {
            resolve(lastKnownLocation);
            return;
          }

          // You are setting lastKnownLocation to a new value
          lastKnownLocation = {
            latitude: pos.coords.latitude,
            longitude: pos.coords.longitude,
          };
          // Therefore, set the lastKnownLookup to null so that it is retrieved
          lastKnownLookup = null;
          resolve(lastKnownLocation);
        },
        (e) => reject('Geolocation error: (' + e.code + '): ' + e.message),
        location_request_options
      );
    });
  }

  function fetch() {
    return requestChain.execute(
      requestLocation,
      refreshClimateAndLocation,
      clearClimateAndLocation
    );
  }

  function scheduleNextRefresh() {
    if (waitTimeoutHandle) {
      clearTimeout(waitTimeoutHandle);
      waitTimeoutHandle = null;
    }
    waitTimeoutHandle = setTimeout(fetchWaitRepeat, waitInterval * 60 * 1000);
  }

  function fetchUpdate() {
    if (waitTimeoutHandle) {
      clearTimeout(waitTimeoutHandle);
      waitTimeoutHandle = null;
    }
    fetch().then(scheduleNextRefresh);
  }

  function fetchWaitRepeat() {
    // Check if BT connection is alive
    if (sendUserMsg && typeof sendUserMsg === 'function') {
      // Set max-attempts to 3
      // If the ping fails, you still want to schedule the next refresh
      sendUserMsg({ PING: 1 }, fetchUpdate, scheduleNextRefresh, 3);
    } else {
      // Possible reasons we get here:
      // 1. First time fetching an update
      // 2. No sendUserMsg was provided during creation
      fetchUpdate();
    }
  }

  function kickoffUpdates(minutes) {
    var changed = waitInterval !== minutes;
    if (changed || waitTimeoutHandle === null) {
      waitInterval = minutes;
      fetchWaitRepeat();
    } else {
      // Interval unchanged and waitTimeoutHandle is valid
      oneShotCachedClimateAndLocation();
    }
  }

  return kickoffUpdates;
}

class ClimateLocationRequestQueue {
  constructor() {
    this.chain = Promise.resolve();
  }

  // This structure is the right one because we handle the catch
  // as well as success conditions. Therefore, the request chain
  // always have a fulfilled promise.
  // Additionally, the then() block of the caller always gets
  // executed vs. needing an error block, so the next timer is set
  // durably in all situations.
  execute(coords, refresh, clear) {
    this.chain = this.chain
      .then(() => {
        return coords();
      })
      .then((location) => {
        return refresh(location);
      })
      .catch(clear);
    return this.chain;
  }
}

module.exports = {
  createRefreshController: createRefreshController,
};
