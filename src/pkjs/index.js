// Clay stuff
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var messageKeys = require('message_keys');
var createAppMessageQueue = require('./app_message_queue').createAppMessageQueue;
var createRefreshController = require('./weather_location').createRefreshController;

const STEPS_GOAL_MIN = 4000;
const STEPS_GOAL_DEFAULT = 10000;
const STEPS_GOAL_MAX = 32000;

// 15-minute default
const WEATHER_UPDATE_MINUTES_DEFAULT = 15;

// Max-update interval is 6-hours
const WEATHER_UPDATE_MINUTES_MIN = 15;
const WEATHER_UPDATE_MINUTES_MAX = 60;
function sendPebbleMessage(payload, success, failure) {
  Pebble.sendAppMessage(payload, success, failure);
}

var updateUser = createAppMessageQueue(sendPebbleMessage);
var updateClimateLocation = createAppMessageQueue(sendPebbleMessage);

function clampStepsGoal(goal) {
  if (goal < STEPS_GOAL_MIN) {
    return STEPS_GOAL_MIN;
  }
  if (goal > STEPS_GOAL_MAX) {
    return STEPS_GOAL_MAX;
  }
  return goal;
}

function calculateStepsGoal(settings) {
  var goal = STEPS_GOAL_DEFAULT;
  if (settings) {
    var presetGoal = parseInt(settings[messageKeys.STEPS_GOAL_PRESET], 10);
    var customGoal = parseInt(settings[messageKeys.STEPS_GOAL_CUSTOM], 10);

    if (!isNaN(customGoal)) {
      goal = clampStepsGoal(customGoal);
    } else if (!isNaN(presetGoal)) {
      goal = clampStepsGoal(presetGoal);
    }
  }

  return goal;
}

// Return the default value if setting is invalid or irretrievable
function validateWeatherUpdateMinutes(input) {
  var toReturn = parseInt(input, 10);
  toReturn = isNaN(toReturn) ? WEATHER_UPDATE_MINUTES_DEFAULT : toReturn;
  toReturn = toReturn <= WEATHER_UPDATE_MINUTES_MAX ? toReturn : WEATHER_UPDATE_MINUTES_MAX;
  return toReturn >= WEATHER_UPDATE_MINUTES_MIN ? toReturn : WEATHER_UPDATE_MINUTES_MIN;
}

var clay = new Clay(clayConfig, null, {
  autoHandleEvents: false,
});
// var startRefresh = createRefreshController(updateClimateLocation);
var requestUpdate = createRefreshController(updateClimateLocation);

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('appmessage', function (e) {
  if (!e || !e.payload) {
    return;
  }
  if (e.payload.REFRESH && e.payload.REFRESH === 1) {
    // Refresh climate and location
    requestUpdate();
  }
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    return;
  }

  // Retrieves settings as a dictionary with message_keys
  // Ready to send via Pebble's app message to the watch face
  // so, messageKeys.KEY_NAME
  var dict = clay.getSettings(e.response);

  dict[messageKeys.STEPS_GOAL] = calculateStepsGoal(dict);
  dict[messageKeys.WEATHER_UPDATE_MINUTES] = validateWeatherUpdateMinutes(
    dict[messageKeys.WEATHER_UPDATE_MINUTES]
  );
  delete dict[messageKeys.STEPS_GOAL_PRESET];
  delete dict[messageKeys.STEPS_GOAL_CUSTOM];
  updateUser(
    dict,
    function () {
      console.log('Sent config data to Pebble');
    },
    function (e) {
      console.log('Config sent failed with error: (' + e.code + '): ' + e.message);
    }
  );
  // If settings have changed, C will request a refresh
});

Pebble.addEventListener('ready', function () {
  try {
    updateUser({ JS_READY: 1 });
  } catch (e) {
    console.log('Ready Event Handler failed due to error: ' + e.message);
  }
});
