# ft

_ft_ is _from Traccar_. It listens on the loopback interface and obtains positions and events from Traccar via HTTP POST, extracts the JSON payload from the body, and publishes it via MQTT.

The topic branch is contructed from the device _uniqueId_ and `position` or `event/<eventType>`

## usage

```bash
export FT_USER=mqttusername
export FT_PASS=mqttpass
```


## topics

```
_traccar/q54/event/deviceOnline {"event":{"id":1183,"attributes":{},"deviceId":7,"type":"deviceOnline","
```

```
_traccar/q54/position {"position":{"id":18321,"attributes":{"t":"i","ignition":true,
```

```
_traccar/q54/event/geofenceEnter {"geofence":{"id":7,"attributes":{},
```


