/*
Generate patterned sensor data and send(Parent Process) with IPC to the other process(Child Process)
IPC Method : PIPE(Half-Duplex)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <sys/wait.h>
#include <sys/ipc.h>

#include <mariadb/mysql.h>
// #include <mqtt/MQTTClient.h>
#include <MQTTClient.h>

// MariaDB Connection parameters^M
#define DB_HOST "localhost"
#define DB_USER "scott"
#define DB_PASSWORD "tiger"
#define DB_NAME "mydb"

#define MQTT_BROKER_ADDRESS "tcp://localhost:1883"
#define MQTT_CLIENT_ID "temperature_sensor_client"
#define MQTT_TOPIC "sensor/temp"


void saveToMariaDB(int SID, const char *value)
{
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL)
    {
        fprintf(stderr, "mysql_init() failed\n");
        exit(EXIT_FAILURE);
    }

    if (mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, 0, NULL, 0) == NULL)
    {
        fprintf(stderr, "mysql_real_connect() failed\n");
        mysql_close(conn);
        exit(EXIT_FAILURE);
    }

    char query[256];
    sprintf(query, "INSERT INTO SensorData (sensor_id, reading, timestamp) VALUES (%d, '%s', sysdate())", SID, value);

    if (mysql_query(conn, query) != 0)
    {
        fprintf(stderr, "MariaDB query execution failed: %s\n", mysql_error(conn));
    }

    mysql_close(conn);
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
    // char senval[2][10];

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
        while (1)
        {
            if (read(pipefd[0], receivedString, sizeof(receivedString)) > 0)
            {
                // Process the received string value
                // receivedString[strcspn(receivedString, "\n")] = 0;
                printf("Consumer received: %s\n", receivedString);

                // char *token = strtok((char *)receivedString, ":");

                // Keep printing tokens while one of the
                // delimiters present in str[].

                // int i = 0;
                // while (token != NULL)
                // {
                //     strcpy(senval[i], token);
                //     printf("%s\n", token);
                //     token = strtok(NULL, ":");
                //     i++;
                // }

                // for (int i = 0; i < 2; i++)
                // {
                //     // Save to MariaDB
                //     // saveToMariaDB(receivedString);
                //     saveToMariaDB((i + 1), senval[i]);

                //     // Send to MQTT
                //     // sendToMQTT(receivedString);
                //     sendToMQTT(topics[i], senval[i]);
                // }

                saveToMariaDB(1, receivedString);
                sendToMQTT(MQTT_TOPIC, receivedString);
            }
        }

        // Close read end of the pipe
        close(pipefd[0]);
    }
    else
    {
        // Parent process (virtual_sensor_data_generator)
        double temperature = 0.0f;

        // Close read end of the pipe
        close(pipefd[0]);

        // Redirect standard output to the write end of the pipe
        dup2(pipefd[1], STDOUT_FILENO);

        // Generate random sensor data
        while(1) 
        {
            // printf("%.2f:%.0f\n", temperature, humidity);
            temperature = (double)((rand() % 1201) - 600) / 100; // -6.00'C ~ 6.00'C
            printf("%.2f\n", temperature);
            fflush(stdout);
            sleep(1);
        }

        // Close write end of the pipe
        close(pipefd[1]);
    }

    return 0;
}