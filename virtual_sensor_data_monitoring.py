from flask import Flask, render_template
from flask_socketio import SocketIO, emit
import pymysql
import pandas as pd

import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from pandas import DataFrame
from random import randrange
from sqlalchemy import create_engine


sensor_type=["Temperature", "Humidity", "CO2"]
plot_color=["#F44336", "#2196F3", "#1F2A38"]

app=Flask(__name__)
socketio=SocketIO(app,cors_allowed_origins="*")


def get_sensor_data(index):
    global sensor_type
    # conn = pymysql.connect(host='localhost',user='scott',password='tiger',database='mydb')
    engine = create_engine('mysql+pymysql://scott:tiger@localhost/mydb')
    query=("SELECT TodaySensorData.timestamp, TodaySensorData.reading " 
           "FROM (SELECT * FROM SensorData WHERE DATE(timestamp) = CURDATE() ORDER BY id DESC LIMIT 100) AS TodaySensorData "
           "JOIN Sensors ON TodaySensorData.sensor_id = Sensors.id "
           f"WHERE Sensors.name = '{sensor_type[index]}' "
           "ORDER BY TodaySensorData.id ASC;")
    df=pd.read_sql(query,con=engine)
    df=df.set_index('timestamp')
    # conn.close()
    engine.dispose()
    return df

def generate_plot(df, index):
    global plot_color
    return df.plot(use_index=True,y=["reading"],
            kind="line",figsize=(6,4), color=plot_color[index]).legend(loc='upper left')

@socketio.on('connect')
def handle_connect():
    print('Client connected')
    emit('update_plot','Connected')

@socketio.on('get_data')
def handle_get_data():
    global sensor_type
    data=""
    # get plot
    for i in range(3):
        sensor_data=get_sensor_data(i)
        data+=(sensor_data.tail(1)).to_string(index=False, header=False)
        if i != 2: 
            data+=':'
        plot=generate_plot(sensor_data, i)
        plt.title(sensor_type[i])
        plt.savefig(f'static/plots/plot{i}.png')
        plt.close()
    emit('update', data)


@app.route('/')
def index():
    return render_template("index_websocket.html")


if __name__ == '__main__':
    socketio.run(app,port=5000)