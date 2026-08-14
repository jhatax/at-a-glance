// Clay stuff
var Clay = require('@rebble/clay');
var clayConfig = require('./config.json');
var messageKeys = require('message_keys');
var createAppMessageQueue = require('./app_message_queue').createAppMessageQueue;
var createRefreshController = require('./weather_location').createRefreshController;

const STEPS_GOAL_MIN = 4000;
const STEPS_GOAL_DEFAULT = 10000;
const STEPS_GOAL_MAX = 32000;

const WEATHER_UPDATE_MINUTES_DEFAULT = 15;
function sendPebbleMessage(payload, success, failure) {
  Pebble.sendAppMessage(payload, success, failure);
}

var sendWeatherMessage = createAppMessageQueue(sendPebbleMessage);
var sendLocationMessage = createAppMessageQueue(sendPebbleMessage);
var sendAppMessage = createAppMessageQueue(sendPebbleMessage);

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

// Return the default value if setting is invalid or irretrievable
function parseWeatherUpdateMinutes(rawSettings) {
  var toreturn = rawSettings
    ? parseInt(rawSettings.WEATHER_UPDATE_MINUTES, 10)
    : WEATHER_UPDATE_MINUTES_DEFAULT;
  return isNaN(toreturn) ? WEATHER_UPDATE_MINUTES_DEFAULT : toreturn;
}

function applyWeatherUpdateMinutes(minutes) {
  refresh.setIntervalMinutes(isNaN(minutes) ? WEATHER_UPDATE_MINUTES_DEFAULT : minutes);
}

var clay = new Clay(clayConfig);
var refresh = createRefreshController({
  sendWeatherMessage: sendWeatherMessage,
  sendLocationMessage: sendLocationMessage,
});

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
  sendAppMessage(
    dict,
    function () {
      console.log('Sent config data to Pebble');
    },
    function (e) {
      console.log('Config sent failed with error: ' + JSON.stringify(e));
    }
  );
});

Pebble.addEventListener('ready', function () {
  sendAppMessage({ JS_READY: 1 });
  refresh.start(WEATHER_UPDATE_MINUTES_DEFAULT);
});
