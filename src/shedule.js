

var sentSuccessfully = function(e) {
  console.log('Shedule sent to Pebble successfully!');
};

var sentUnsuccessfully = function(e) {
  console.log('Error sending shedule to Pebble!');
};

var trains = [];

var sendFeed = function(data) {
  var currentDate = new Date();
  var timezoneOffset = (new Date()).getTimezoneOffset() * 60;

  var j = 0;
  for(var i = 0; i < data.threads.length; i++) {

    var departureDate = new Date(data.threads[i].departure);

    if (currentDate < departureDate) {

      var dtpartureDateUTC = departureDate.getTime() / 1000 - timezoneOffset;
      var title = data.threads[i].thread.short_title;

      trains[j] = {
        'KEY_TRAIN_TITLE': title,
        'KEY_TRAIN_TIME': dtpartureDateUTC,
        'KEY_TRAIN_NUMBER' : j
      };

      j++;
    }
  }

  Pebble.sendAppMessage({
    'KEY_TRAIN_COUNT': trains.length
  }, sentSuccessfully, sentUnsuccessfully);
  
};


function sendSheduleData(){
  var date = new Date();
  console.log('date = ' + date.getFullYear() + '-' + ('0' + (date.getMonth() + 1)).slice(-2) + '-' + ('0' + date.getDate()).slice(-2));// date.toLocaleDateString()); //date.getUTCYear() + '-'+  date.getUTCMonth() + '-' + date.getUTCDate());

  var xhr = new XMLHttpRequest();

  xhr.open("GET", "https://api.rasp.yandex.net/v1.0/search/?apikey=e11e0228-1a80-4090-98a9-046137c4fbb0&format=json&lang=ru&from=s2000001&to=s9601266&date=" + date.getFullYear() + '-' + ('0' + (date.getMonth() + 1)).slice(-2) + '-' + ('0' + date.getDate()).slice(-2), true);
  xhr.withCredentials=true;
  xhr.onload = function (e) {

    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        var sheduleFeed = JSON.parse(decodeURIComponent(xhr.responseText));
        sendFeed(sheduleFeed);
      } else {
        console.error(xhr.statusText);
      }
    }
  };
  xhr.send();
}

function locationSuccess(pos) {

}

function locationError(err) {
  console.log('Error requesting location!');
}

function getShedule() {
  sendSheduleData();

  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log ("reseived: " + e.payload.KEY_COMMAND);
    switch (e.payload.KEY_COMMAND) {

      case "send_next_train":
        if (trains.length > 0) {
          Pebble.sendAppMessage(trains.shift(), sentSuccessfully, sentUnsuccessfully);
        } else {
          Pebble.sendAppMessage({
            'KEY_SHEDULE_SENT': 1,
          }, sentSuccessfully, sentUnsuccessfully);
        }
       break;
      
      case "get_shedule":
        getShedule();
        break;
    }
  }                     
);

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

  // Get the initial weather
    getShedule();
  }
);

