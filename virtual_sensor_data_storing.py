import datetime
import random
import time
import paho.mqtt.client as mqtt
import pymysql

# topic
sensor_type=["Temperature", "Humidity", "CO2"]
symbol=["°C","%","ppm"]

def on_message(client, userdata, message):
    global sensor_type
    global symbol
    try:
        payload = message.payload.decode()
        data = eval(payload)

        timestamp = datetime.datetime.strptime(data['time'], '%Y-%m-%d %H:%M:%S')
        sensor_id = sensor_type.index(message.topic)
        reading = data['reading']


        db_connection = pymysql.connect(host=db_host,user=db_user,password=db_password,database=db_name)
        cursor = db_connection.cursor()

        query=f"INSERT INTO {table_name} (sensor_id, reading, timestamp) VALUES (%s,%s,%s)"
        cursor.execute(query,(sensor_id + 1, reading, timestamp))
        db_connection.commit()

        db_connection.close()
        print(f"Received and stored / Sensor : {sensor_type[sensor_id]}, Reading : {reading+symbol[sensor_id]}, Time : {timestamp}")
    except Exception as e:
        print(f"Error:{e}")

broker_address = 'tcp://localhost'
broker_port = 1883

db_host = 'localhost'
db_user = 'scott'
db_password = 'tiger'
db_name = 'mydb'
table_name = 'SensorData'

mqtt_client = mqtt.Client()
mqtt_client.connect(broker_address, broker_port)

mqtt_client.subscribe(topic)
mqtt_client.on_message = on_message
mqtt_client.loop_start()