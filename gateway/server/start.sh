#!/bin/sh

python3 -m webbrowser -t "file://$(pwd)/index.html"

if [[ ${OSTYPE//[0-9.]/} = "darwin" ]] 
then
    IP=$(route get 8.8.8.8 | grep gateway | sed 's/ *gateway: //g')
    echo "===== IP_ADDRESS: $IP ====="
    /opt/homebrew/opt/mosquitto/sbin/mosquitto -c mosquitto.conf
else
    IP=$(ip route get 8.8.8.8 | grep gateway | sed 's/ *gateway: //g')
    echo "===== IP_ADDRESS: $IP ====="
    mosquitto -c mosquitto.conf
fi

# python3 ./bridge.py