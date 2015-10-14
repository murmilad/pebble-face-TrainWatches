

var sentSuccessfully = function(e) {
  console.log('Shedule sent to Pebble successfully!');
};

var sentUnsuccessfully = function(e) {
  console.log('Error sending shedule to Pebble!');
};

var trains = [];
var timezoneOffset;

function getDateUTC (dateString){
  var date = new Date(dateString);
  return date.getTime() / 1000 - timezoneOffset;
}

function getSheduleData(stationPos, stationHome){
  var date = new Date();
  var dateUTC = date.getTime() / 1000;

  var xhr = new XMLHttpRequest();

  xhr.open("GET", "https://api.rasp.yandex.net/v1.0/search/?apikey=e11e0228-1a80-4090-98a9-046137c4fbb0&format=json&lang=ru&from=" + stationPos.KEY_STATION_CODE + "&to=" + stationHome.KEY_STATION_CODE + "&date=" + date.getFullYear() + '-' + ('0' + (date.getMonth() + 1)).slice(-2) + '-' + ('0' + date.getDate()).slice(-2), false);
  xhr.withCredentials=true;
  xhr.onload = function (e) {

    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        var sheduleFeed = JSON.parse(decodeURIComponent(xhr.responseText));
        sheduleFeed.threads.forEach(function(train, i, arr) {
          var exitTime = getDateUTC(train.departure) - stationPos.KEY_STATION_TIME;
          if (dateUTC < exitTime) {
            console.log('Train ' + train.thread.short_title);
            console.log('Train number' + train.thread.number);
            console.log('Train number' + train.thread.uid);

            trains.push({
              'KEY_TRAIN_TITLE': train.from.title + " - " + train.to.title,
              'KEY_TRAIN_TIME': getDateUTC(train.departure),
              'KEY_TRAIN_LINE': train.thread.number,
              'KEY_EXIT_TIME' : exitTime,
              'KEY_TRACK_TITLE' : train.thread.short_title,
              'KEY_STATION_DISTANCE': stationPos.KEY_STATION_DISTANCE,
              'KEY_HOME_DISTANCE': stationHome.KEY_STATION_DISTANCE,
              'KEY_TRAIN_NUMBER': 0,
            });
          }
        });
      } else {
        console.error(xhr.statusText);
      }
    }
  };

  xhr.onerror = function (e) {
    console.error(xhr.statusText);
  };

  xhr.send();
  
}

function getColsestStations(lat, lng) {
  var xhr = new XMLHttpRequest();
  var stations = [];

  xhr.open("GET", "https://api.rasp.yandex.net/v1.0/nearest_stations/?&apikey=e11e0228-1a80-4090-98a9-046137c4fbb0&format=json&lat=" + lat + "&lng=" + lng + "&distance=2&lang=ru&transport_types=train", false);
  xhr.withCredentials=true;
  xhr.onload = function (e) {

    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        console.log("xhr.status === 200");
        var configuration = JSON.parse(decodeURIComponent(xhr.responseText));
        configuration.stations.forEach(function(station, i, arr) {
          console.log('Station ' + station.title);
          console.log('Station dest ' + station.distance);
          console.log('Station time ' + Math.round(station.distance / 5 * 60 * 60));

          stations.push({
            'KEY_STATION_TITLE': station.title,
            'KEY_STATION_DISTANCE': Math.round(station.distance * 1000),
            'KEY_STATION_TIME': Math.round(station.distance / 5 * 60 * 60),
            'KEY_STATION_CODE': station.code,
          });
        });
      } else {
        console.error(xhr.statusText);
      }
    }
  };

  xhr.onerror = function (e) {
    console.error(xhr.statusText);
  };

  xhr.send();

  return stations;
}

function locationSuccess(pos) {
  trains = [];

  console.log('Get pos stations');

  var stationsPos  = getColsestStations(pos.coords.latitude, pos.coords.longitude);

  console.log('Get home stations');

  var stationsHome = getColsestStations(localStorage.getItem('home_lat'), localStorage.getItem('home_lng'));
  stationsPos.forEach(function(stationPos, i, arrPos) {
    stationsHome.forEach(function(stationHome, j, arrHome) {
      getSheduleData(stationPos, stationHome);
    });
  });
  trains.sort(function(a, b) {
    return a.KEY_TRAIN_LINE.localeCompare(b.KEY_TRAIN_LINE) || a.KEY_STATION_DISTANCE - b.KEY_STATION_DISTANCE || a.KEY_HOME_DISTANCE - b.KEY_HOME_DISTANCE;
  });
  
  var trainLine = "";
  trains = trains.filter(function(train) {
    var isUnic = train.KEY_TRAIN_LINE !=  trainLine;
    trainLine = train.KEY_TRAIN_LINE;
    return isUnic;
  });

  trains.sort(function(a, b) {
    return a.KEY_EXIT_TIME - b.KEY_EXIT_TIME;
  });

  trains.forEach(function(train, i, arr) {
    train.KEY_TRAIN_NUMBER = i;
  });
  
  
  console.log('Train' + JSON.stringify(trains));

  Pebble.sendAppMessage({
    'KEY_TRAIN_COUNT': trains.length
  }, sentSuccessfully, sentUnsuccessfully);
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getShedule() {

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
        console.log ("send_next_train ");
        console.log ("trains.length " + trains.length);
        if (trains.length > 0) {
          console.log ("send");
          Pebble.sendAppMessage(trains.shift(), sentSuccessfully, sentUnsuccessfully);
          console.log ("sent");
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
    timezoneOffset = (new Date()).getTimezoneOffset() * 60;

    getShedule();
  }
);

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL('http://akosarev.info/pebble/tw/config/index.html');
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode and parse config data as JSON
  var config_data = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + e.response);

  localStorage.setItem('home_lat', config_data.lat);
  localStorage.setItem('home_lng', config_data.lng);
  
  getShedule();
});

