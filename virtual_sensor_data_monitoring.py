from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import pymysql
import pandas as pd

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pandas import DataFrame
from random import randrange
from sqlalchemy import create_engine

# Subscribe to MQTT Broker
import paho.mqtt.client as mqtt

broker_address = '127.0.0.1'
broker_port = 1883
topic = 'sensor/temp'

app=Flask(__name__)
socketio=SocketIO(app,cors_allowed_origins="*")

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT broker")
    client.subscribe(topic)

def on_message(client, userdata, msg):
    print(f"Received:{msg.payload.decode()}")
    socketio.send('Server received: ' + msg.payload.decode())

client = mqtt.Client()

client.on_connect = on_connect
client.on_message = on_message

client.connect(broker_address, broker_port)
client.loop_start()

@socketio.on('connect')
def handle_connect():
    print('Client connected')

@app.route('/')
def index():
    return render_template("index_websocket.html")    

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)