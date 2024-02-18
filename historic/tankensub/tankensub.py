#!/usr/bin/env python -B
# -*- coding: utf-8 -*-

# tankensub.py (Python3) subscribes to MQTT and reads JSON payloads on topics
# produced by ft (from-traccar). The lat/lon is extracted, and tankensub checks
# whether the position is near a fuel station.

import paho.mqtt.client as paho   # pip install paho-mqtt
import socket
import json
import geohash
import os, sys
import datetime
import time
import proximityhash
import cdbx
from haversine import haversine

GEO_PREC = 8

cdb = cdbx.CDB(os.getenv("DATA_FILE"))

__author__    = 'Jan-Piet Mens <jp@mens.de>'
__copyright__ = 'Copyright 2018-2024 Jan-Piet Mens'
__license__   = """GNU General Public License"""

def on_connect(mosq, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        print(f"Failed to connect! flags:{flags} reason_code:{reason_code} properties:{properties}. loop_forever() will retry connection")
    else:
        print(f"Connected! flags:{flags} reason_code:{reason_code} properties:{properties}")
        mqttc.subscribe("_traccar/+/event/ignitionOff", 0)

def on_message(mosq, userdata, msg):
    # print("%s (qos=%s, r=%s) %s" % (msg.topic, str(msg.qos), msg.retain, str(msg.payload)))

    '''
        "position": {
                "id": 18300,
                "attributes": {
                        "t": "I",
                        "ignition": false,
                        "distance": 0,
                        "totalDistance": 28212813.97,
                        "motion": false,
                        "hours": 55000
                },
                "deviceId": 7,
                "protocol": "owntracks",
                "serverTime": null,
                "deviceTime": "2018-09-10T07:47:45.000+0000",
                "fixTime": "2018-09-10T07:47:45.000+0000",
                "outdated": false,
                "valid": true,
                "latitude": 49.0156556,
                "longitude": 8.3975169,
                "altitude": 0,
    '''

    try:
        d = json.loads(msg.payload)
    except:
        return

    if 'position' in d:
        p = d['position']
        if 'latitude' not in p or 'longitude' not in p:
            return

    olat = lat = float(p['latitude'])
    olon = lon = float(p['longitude'])

    print("lat=", lat, "lon=", lon)
    ghash = geohash.encode(lat, lon, GEO_PREC)
    t = datetime.datetime.now()
    R=''

    if ghash in cdb:
        data = json.loads(cdb[ghash])
        print(t, "R=", R, msg.topic, ghash, json.dumps(data, indent=4))
    else:
        hash_list = proximityhash.create_geohash(lat, lon, 100, 7).split(',')
        for neighbor in hash_list:
            # print("--->", neighbor)
            lat, lon, a, b = geohash.decode_exactly(neighbor)
            R = haversine(olon, olat, lon, lat) * 1000.0
            # print("N=",neighbor, lat, ",", lon)
            if neighbor in cdb:
                data = json.loads(cdb[neighbor])
                print(t, "R=%6.1fm" % R, msg.topic, neighbor, json.dumps(data, indent=4))

def on_disconnect(client, userdata, flags, reason_code, properties):
    print(f"Disconnected! flags:{flags} reason_code:{reason_code} properties:{properties}")

clientid = 'tankendings-%s' % os.getpid()
protocol=paho.MQTTv31  # 3
protocol=paho.MQTTv311 # 4

mqttc = paho.Client(paho.CallbackAPIVersion.VERSION2, clientid, clean_session=True, userdata=None, protocol=protocol)
mqttc.on_message = on_message
mqttc.on_connect = on_connect
mqttc.on_disconnect = on_disconnect

if os.getenv("MQTT_USER"):
    mqttc.username_pw_set(os.getenv("MQTT_USER"), os.getenv("MQTT_PASS"))

mqttc.connect(os.getenv("MQTT_HOST"), int(os.getenv("MQTT_PORT")), 60)

while True:
    try:
        mqttc.loop_forever()
    except socket.error:
        print("MQTT server disconnected; sleeping")
        time.sleep(5)
    except KeyboardInterrupt:
        mqttc.disconnect()
        sys.exit(0)
    except:
        raise
