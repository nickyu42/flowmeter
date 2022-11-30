import re
import json

import serial
import paho.mqtt.client as mqtt

msg_pattern = re.compile(r'({.*})\n\r')

client = mqtt.Client()
client.connect('localhost', 9001, 60)

with serial.Serial('/dev/tty.wchusbserial54F90171571', 115200) as ser:
	while True:
		line = ser.readline()
		client.publish('/flow', line)
