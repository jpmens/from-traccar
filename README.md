# ft

_ft_ is _from Traccar_. It listens on the loopback interface and obtains positions and events from Traccar via HTTP POST, extracts the JSON payload from the body, and publishes it via MQTT.

![flow](flow.png)

The topic branch is contructed from the device _uniqueId_ and `position` or `event/<eventType>`

## usage

```bash
export FT_USER=mqttusername
export FT_PASS=mqttpass
```


## topics

```
_traccar/q54/position
_traccar/q54/event/geofenceExit
_traccar/q54/event/ignitionOff
```


```json
{
	"event": {
		"id": 1187,
		"attributes": {},
		"deviceId": 7,
		"type": "deviceOnline",
		"serverTime": "2018-09-14T10:18:32.285+0000",
		"positionId": 0,
		"geofenceId": 0,
		"maintenanceId": 0
	},
	"device": {
		"id": 7,
		"attributes": {
			"aaa": "AAAA",
			"bbb": "BBBB",
			"ccc": "CCCC"
		},
		"groupId": 0,
		"name": "Demo-54",
		"uniqueId": "q54",
		"status": "online",
		"lastUpdate": "2018-09-14T10:18:32.285+0000",
		"positionId": 18321,
		"geofenceIds": [
			7
		],
		"phone": "0171",
		"model": "dkdkk",
		"contact": "contcact",
		"category": "boat",
		"disabled": false
	},
	"users": [
		{
			"id": 1,
			"attributes": {
				"pushover.user": "a22200",
				"pushover,token": "aabbbt0009"
			},
			"name": "admin",
			"login": "",
			"email": "admin",
			"phone": "+491234567890",
			"readonly": false,
			"administrator": true,
			"map": "",
			"latitude": 0,
			"longitude": 0,
			"zoom": 3,
			"twelveHourFormat": false,
			"coordinateFormat": "",
			"disabled": false,
			"expirationTime": null,
			"deviceLimit": -1,
			"userLimit": 0,
			"deviceReadonly": false,
			"token": null,
			"limitCommands": false,
			"poiLayer": "",
			"password": null
		}
	]
}
{
	"position": {
		"id": 18322,
		"attributes": {
			"t": "I",
			"ignition": false,
			"distance": 449672.22,
			"totalDistance": 29561830.63,
			"motion": false,
			"hours": 55000
		},
		"deviceId": 7,
		"type": null,
		"protocol": "owntracks",
		"serverTime": null,
		"deviceTime": "2018-09-14T10:18:32.000+0000",
		"fixTime": "2018-09-14T10:18:32.000+0000",
		"outdated": false,
		"valid": true,
		"latitude": 48.83875,
		"longitude": 2.25353,
		"altitude": 0,
		"speed": 0,
		"course": 0,
		"address": null,
		"accuracy": 0,
		"network": null
	},
	"device": {
		"id": 7,
		"attributes": {
			"aaa": "AAAA",
			"bbb": "BBBB",
			"ccc": "CCCC"
		},
		"groupId": 0,
		"name": "Demo-54",
		"uniqueId": "q54",
		"status": "online",
		"lastUpdate": "2018-09-14T10:18:32.285+0000",
		"positionId": 18321,
		"geofenceIds": [],
		"phone": "0171",
		"model": "dkdkk",
		"contact": "contcact",
		"category": "boat",
		"disabled": false
	}
}
{
	"geofence": {
		"id": 7,
		"attributes": {},
		"calendarId": 0,
		"name": "blub9",
		"description": "",
		"area": "CIRCLE (49.133867934876974 8.166520803303387, 33112.6)"
	},
	"position": {
		"id": 18322,
		"attributes": {
			"t": "I",
			"ignition": false,
			"distance": 449672.22,
			"totalDistance": 29561830.63,
			"motion": false,
			"hours": 55000
		},
		"deviceId": 7,
		"type": null,
		"protocol": "owntracks",
		"serverTime": null,
		"deviceTime": "2018-09-14T10:18:32.000+0000",
		"fixTime": "2018-09-14T10:18:32.000+0000",
		"outdated": false,
		"valid": true,
		"latitude": 48.83875,
		"longitude": 2.25353,
		"altitude": 0,
		"speed": 0,
		"course": 0,
		"address": null,
		"accuracy": 0,
		"network": null
	},
	"event": {
		"id": 1188,
		"attributes": {},
		"deviceId": 7,
		"type": "geofenceExit",
		"serverTime": "2018-09-14T10:18:32.316+0000",
		"positionId": 18322,
		"geofenceId": 7,
		"maintenanceId": 0
	},
	"device": {
		"id": 7,
		"attributes": {
			"aaa": "AAAA",
			"bbb": "BBBB",
			"ccc": "CCCC"
		},
		"groupId": 0,
		"name": "Demo-54",
		"uniqueId": "q54",
		"status": "online",
		"lastUpdate": "2018-09-14T10:18:32.285+0000",
		"positionId": 18321,
		"geofenceIds": [],
		"phone": "0171",
		"model": "dkdkk",
		"contact": "contcact",
		"category": "boat",
		"disabled": false
	},
	"users": [
		{
			"id": 1,
			"attributes": {
				"pushover.user": "a22200",
				"pushover,token": "aabbbt0009"
			},
			"name": "admin",
			"login": "",
			"email": "admin",
			"phone": "+491234567890",
			"readonly": false,
			"administrator": true,
			"map": "",
			"latitude": 0,
			"longitude": 0,
			"zoom": 3,
			"twelveHourFormat": false,
			"coordinateFormat": "",
			"disabled": false,
			"expirationTime": null,
			"deviceLimit": -1,
			"userLimit": 0,
			"deviceReadonly": false,
			"token": null,
			"limitCommands": false,
			"poiLayer": "",
			"password": null
		}
	]
}
```
