

var sentSuccessfully = function(e) {
  console.log('Shedule sent to Pebble successfully!');
};

var sentUnsuccessfully = function(e) {
  console.log('Error sending shedule to Pebble!');
};

var trains = [];
var stations = [];
var timezoneOffset;
var stationsHome = [];
var stationsPos  = [];

var date = new Date();
var dateUTC = date.getTime() / 1000;
var xhr;

//This function takes in latitude and longitude of two location and returns the distance between them as the crow flies (in km)
function calcSearchDistance(lat1, lon1, lat2, lon2) {
  var R = 6371; // km
  var dLat = toRad(lat2-lat1);
  var dLon = toRad(lon2-lon1);
  lat1 = toRad(lat1);
  lat2 = toRad(lat2);

  var a = Math.sin(dLat/2) * Math.sin(dLat/2) +
      Math.sin(dLon/2) * Math.sin(dLon/2) * Math.cos(lat1) * Math.cos(lat2); 
  var c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a)); 
  var d = R * c;
  
  
  return Math.min(Math.max(Math.floor(d / 10), 1),4);
}

// Converts numeric degrees to radians
function toRad(Value) 
{
  return Value * Math.PI / 180;
}

function getDateUTC (dateString){
  var date = new Date(dateString);
  return date.getTime() / 1000 - timezoneOffset;
}

function getSheduleData(stationPos, stationHome, dateStr){
  xhr = new XMLHttpRequest();

  xhr.open("GET", "https://api.rasp.yandex.net/v1.0/search/?apikey=e11e0228-1a80-4090-98a9-046137c4fbb0&format=json&lang=ru&from=" + stationPos.KEY_STATION_CODE + "&to=" + stationHome.KEY_STATION_CODE + "&date=" + dateStr, false);
           
  xhr.onload = function (e) {

    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        console.log('Json b ' +  stationPos.KEY_STATION_TITLE + "to" + stationHome.KEY_STATION_TITLE);
        var sheduleFeed = JSON.parse(decodeURIComponent(xhr.responseText));
        console.log('Json a ' +  stationPos.KEY_STATION_TITLE + "to" + stationHome.KEY_STATION_TITLE);
        sheduleFeed.threads.forEach(function(train, i, arr) {
          console.log('Train b ' + train.thread.short_title);
          var exitTime = getDateUTC(train.departure) - stationPos.KEY_STATION_TIME;
          console.log('Train a ' + train.thread.short_title);
          if (dateUTC < exitTime) {

            trains.push({
              'KEY_TRAIN_TITLE': train.from.title + " - " + train.to.title,
              'KEY_TRAIN_TIME': (getDateUTC(train.departure)),
              'KEY_TRAIN_LINE': train.thread.number,
              'KEY_EXIT_TIME' : (exitTime),
              'KEY_TRACK_TITLE' : train.thread.short_title,
              'KEY_STATION_DISTANCE': stationPos.KEY_STATION_DISTANCE,
              'KEY_HOME_DISTANCE': stationHome.KEY_STATION_DISTANCE,
              'KEY_TRAIN_NUMBER': 0,
              'KEY_TRAIN_STATION_FROM' : stationPos.KEY_STATION_NUMBER,
              'KEY_TRAIN_STATION_TO' : stationHome.KEY_STATION_NUMBER,
            });
          }
        });
      } else {
        console.error(xhr.statusText);
      }
    }
  };

  xhr.send();
}


function getColsestStations(lat, lng, distance) {
  var _stations = [];

  var xhr = new XMLHttpRequest();

  xhr.open("GET", "https://api.rasp.yandex.net/v1.0/nearest_stations/?&apikey=e11e0228-1a80-4090-98a9-046137c4fbb0&format=json&lat=" + lat + "&lng=" + lng + "&distance=" + distance + "&lang=ru&transport_types=train", false);
  xhr.onload = function (e) {

    if (xhr.readyState === 4) {
      if (xhr.status === 200) {
        console.log("xhr.status === 200");
        var configuration = JSON.parse(decodeURIComponent(xhr.responseText));
        configuration.stations.forEach(function(station, i, arr) {

          _stations.push({
            'KEY_STATION_TITLE': station.title,
            'KEY_STATION_DISTANCE': Math.round(station.distance * 1000),
            'KEY_STATION_TIME': Math.round(station.distance / 3 * 60 * 60),
            'KEY_STATION_CODE': station.code,
            'KEY_STATION_NUMBER': stations.length,
          });
          stations.push({
            'KEY_STATION_TITLE': station.title,
            'KEY_STATION_NUMBER': stations.length,
          });
        });
      } else {
        console.error(xhr.statusText);
      }
    }
  };

  xhr.send();
  xhr.abort();

  return _stations;
}

function locationSuccess(pos) {
  trains = [];

  
  var searchDistance = calcSearchDistance(pos.coords.latitude, pos.coords.longitude, localStorage.getItem('home_lat'), localStorage.getItem('home_lng'));
  console.log('Get pos stations');
  stationsPos  = getColsestStations(pos.coords.latitude, pos.coords.longitude, searchDistance);
  console.log('Get home stations');
  stationsHome = getColsestStations(localStorage.getItem('home_lat'), localStorage.getItem('home_lng'), searchDistance);

  Pebble.sendAppMessage({
    'KEY_STATION_COUNT': stations.length
  }, sentSuccessfully, sentUnsuccessfully);

  
}

function sortSheduleData(){
  var dateStr = date.getFullYear() + '-' + ('0' + (date.getMonth() + 1)).slice(-2) + '-' + ('0' + date.getDate()).slice(-2);

  stationsPos.forEach(function(stationPos, i, arrPos) {
    stationsHome.forEach(function(stationHome, j, arrHome) {
      console.log('Get');

      getSheduleData(stationPos, stationHome, dateStr);
    });
  });

  console.log('Find');

  trains.sort(function(a, b) {
    return a.KEY_TRAIN_LINE.localeCompare(b.KEY_TRAIN_LINE) || a.KEY_STATION_DISTANCE - b.KEY_STATION_DISTANCE || a.KEY_HOME_DISTANCE - b.KEY_HOME_DISTANCE;
  });

  console.log('Cut');

  var trainLine = "";
  trains = trains.filter(function(train) {
    var isUnic = train.KEY_TRAIN_LINE !=  trainLine;
    trainLine = train.KEY_TRAIN_LINE;
    return isUnic;
  });

  console.log('Sort');

  trains.sort(function(a, b) {
    return a.KEY_EXIT_TIME - b.KEY_EXIT_TIME;
  });

  console.log('Num');

  trains.forEach(function(train, i, arr) {
    train.KEY_TRAIN_NUMBER = i;
  });
  
  
  console.log('Train ok');

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

      case "send_next_station":
        if (stations.length > 0) {
          console.log ("send st");
          Pebble.sendAppMessage(stations.shift(), sentSuccessfully, sentUnsuccessfully);
        } else {
          sortSheduleData();
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

