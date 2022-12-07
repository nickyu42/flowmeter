#!/usr/bin/env python3

import re
import argparse

import serial
import paho.mqtt.client as mqtt

msg_pattern = re.compile(rb'({.*})\r\n')

parser = argparse.ArgumentParser()
parser.add_argument('-f', '--file')
args = parser.parse_args()

log_file = None
if args.file is not None:
	log_file = open(args.file, 'a+')

print('Connect to MQTT')
client = mqtt.Client()
client.connect('localhost', 1883, 60)

print('Connect to serial')
with serial.Serial('/dev/tty.usbmodem11303', 115200) as ser:
	while True:
		line = ser.readline()
		match = msg_pattern.search(line)

		if match is not None:
			client.publish('/flow', match.group(1))

			if log_file is not None:
				log_file.write(match.group(1).decode('utf-8') + '\n')
