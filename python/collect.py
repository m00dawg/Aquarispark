#!/usr/bin/env python
import paho.mqtt.client as mqtt
from influxdb import InfluxDBClient

temp = None
heater = None
light = None

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("aquarispark/#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    global temp
    global heater
    global light

    influx = InfluxDBClient("localhost", 8086, 'aquarispark', 'Ou6UPpsI01651B1TytTv', 'HomeStats')

    if msg.topic == 'aquarispark/temp':
        temp = msg.payload
        #field = "temp"
    elif msg.topic == 'aquarispark/heater':
        heater = msg.payload
        #field = "heater"
    elif msg.topic == 'aquarispark/light':
        light = msg.payload
        #field = "light"

    if temp is not None and heater is not None and light is not None:
        json_body = [
            {
                "measurement": "Aquarispark",
                "fields": {
                    "temp": float(temp),
                    "heater": int(heater),
                    "light": int(light)
                }
            }
        ]
        influx.write_points(json_body)
        temp = None
        light = None
        heater = None    

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_forever()
