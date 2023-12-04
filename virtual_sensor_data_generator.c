/*
Description : Generate patterned sensor data and send(Parent Process) with IPC to the other process(Child Process)
IPC Method : PIPE(Half-Duplex)
How to compile and build :
1. Install paho.mqtt.c
  a. git clone https://github.com/eclipse/paho.mqtt.c.git
  b. cd paho.mqtt.c
  c. cmake -Bbuild -H. -DPAHO_WITH_SSL=ON
  d. sudo cmake --build build --target install
2. Set up Shared Library Path
  a. sudo find / -name libpaho-mqtt3c.so.1
    ex) /home/user/Lib/paho.mqtt.c/build/src/libpaho-mqtt3c.so.1
  b. export LD_LIBRARY_PATH=/home/user/Lib/paho.mqtt.c/build/src:$LD_LIBRARY_PATH
3. gcc -o virtual_sensor_data_generator virtual_sensor_data_generator.c -lpaho-mqtt3c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/ipc.h>

/*
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON
sudo cmake --build build --target install
*/
#include <MQTTClient.h>
// #include <mqtt/MQTTClient.h>

#define TIME_STRING_LENGTH 20

#define MQTT_BROKER_ADDRESS "tcp://localhost:1883"
#define MQTT_CLIENT_ID "sensor_client"

char* get_current_time_string() {
    time_t current_time;
    struct tm *local_time;
    static char time_str[TIME_STRING_LENGTH]; // Static buffer to hold the formatted time string

    // Get the current time
    current_time = time(NULL);

    // Convert to local time
    local_time = localtime(&current_time);

    // Format the time as a string
    strftime(time_str, TIME_STRING_LENGTH, "%Y-%m-%d %H:%M:%S", local_time);

    // Return the formatted time string
    return time_str;
}

void sendToMQTT(char *topic, const char *value)
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, MQTT_BROKER_ADDRESS, MQTT_CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        fprintf(stderr, "Failed to connect to MQTT broker: %d\n", rc);
        exit(EXIT_FAILURE);
    }
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void *)value;
    pubmsg.payloadlen = strlen(value);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client, topic, &pubmsg, NULL);

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}

int main()
{
    srand(time(NULL));

    int pipefd[2];
    pid_t child_pid;

    // Create a pipe
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Fork a child process
    if ((child_pid = fork()) == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0)
    {
        // Child process (Consumer : Store sensor data with MariaDB and Send with MQTT)

        // Close write end of the pipe
        close(pipefd[1]);

        // Read from the pipe
        char receivedString[100];

        char senval[3][8];
        char topics[3][12] = {"Temperature", "Humidity", "CO2"};
        char json_data[100];

        while (1)
        {
            if (read(pipefd[0], receivedString, sizeof(receivedString)) > 0)
            {
                // Process the received string value
                receivedString[strcspn(receivedString, "\n")] = 0;
                printf("Consumer received: %s\n", receivedString);

                char *token = strtok((char *)receivedString, ":");

                // Keep printing tokens while one of the
                // delimiters present in str[].

                int i = 0;
                while (token != NULL)
                {
                    strcpy(senval[i], token);
                    // printf("%s\n", token);
                    token = strtok(NULL, ":");
                    i++;
                }

                char* current_time_str = get_current_time_string();
                for (int i = 0; i < 3; i++)
                {
                    // Send to MQTT
                    sprintf(json_data, "{\"time\":\"%s\",\"reading\":\"%s\"}", current_time_str, senval[i]);
                    sendToMQTT(topics[i], json_data);
                }
            }
        }

        // Close read end of the pipe
        close(pipefd[0]);
    }
    else
    {
        // Parent process (virtual_sensor_data_generator)
        double temperature = 0.0f;
        double humidity = 0.0f;
        double CO2 = 0.0f;

        // Close read end of the pipe
        close(pipefd[0]);

        // Redirect standard output to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Generate random sensor data
        while(1) 
        {
            temperature = (double)((rand() % 1401) + 1500) / 100; // 15.00'C ~ 29.00'C
            humidity = (rand() % 36) + 60; // 60% ~ 95%
            CO2 = (rand() % 401) + 800; // 800ppm ~ 1200ppm
            printf("%.2f:%.0f:%.0f\n", temperature, humidity, CO2);
            fflush(stdout);
            sleep(1);
        }

        // Close write end of the pipe
        close(pipefd[1]);
    }

    return 0;
}