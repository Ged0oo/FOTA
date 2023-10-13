import paho.mqtt.client as mqtt
import os

# Get the current script's directory
script_dir = os.path.dirname(__file__)

# Change the working directory to the script's directory
os.chdir(script_dir)

# Now you can open 'file.txt' using a relative path
with open('file.txt', 'r') as file:
    file_content = file.read()

# MQTT setup

# Create an MQTT client instance
client = mqtt.Client()

# Set the MQTT broker's address and port
broker_address = "public.mqtthq.com"
port = 1883  # Replace with the actual port number as an integer

# Connect to the MQTT broker
client.connect(broker_address, port)

# Publish the content to an MQTT topic
client.publish('mqttHQ-client-test', file_content)

client.disconnect()
