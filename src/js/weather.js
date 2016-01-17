// Listen for when the watchface is opened
Pebble.addEventListener('ready',
  function(e) {
    console.log('PebbleKit JS ready!');
    // get initial weather info.
    getWeather();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getWeather();
  }
);

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  var myAPIKey = '';
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
  pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;
  console.log('url : ' + url);
  xhrRequest(url, 'GET',
    function(responseText) {
      console.log(responseText);
      var json = JSON.parse(responseText);
      if (json.cod == 200) {
        var temperature = Math.round(json.main.temp - 273.15);
        console.log('temperature is ' + temperature);

        var condition = json.weather[0].main;
        console.log('Conditions is ' + condition);
        
        var conditionId = json.weather[0].id;
        console.log('ConditionId is ' + conditionId);

        var dictionary = {
          'KEY_TEMPERATURE' : temperature,
          'KEY_CONDITION'   : condition,
          'KEY_CONDITION_ID': conditionId
        };

        Pebble.sendAppMessage(dictionary,
          function(e) {
            console.log('Weather info sent to Pebble successfully!');
          },
          function(e) {
            console.log('Error sending weather info to Pebble!');
          }
        );
      } else {
        console.log('cannot get weather info.');
      }
    }
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}
