#ifndef TASK_MQTT
#define TASK_MQTT

#if MQTT_ENABLED == true
    #include <Arduino.h>
    #include <WiFiClientSecure.h>
    #include <MQTTClient.h>
    #include "../config/config.h"

extern unsigned short measurements[];

#define AWS_MAX_MSG_SIZE_BYTES 300

WiFiClientSecure AWS_net;
WiFiClient Wclient;
MQTTClient MQTTcli = MQTTClient(AWS_MAX_MSG_SIZE_BYTES);

// extern const uint8_t aws_root_ca_pem_start[] asm("_binary_certificates_amazonrootca1_pem_start");
// extern const uint8_t aws_root_ca_pem_end[] asm("_binary_certificates_amazonrootca1_pem_end");

// extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificates_certificate_pem_crt_start");
// extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificates_certificate_pem_crt_end");

// extern const uint8_t private_pem_key_start[] asm("_binary_certificates_private_pem_key_start");
// extern const uint8_t private_pem_key_end[] asm("_binary_certificates_private_pem_key_end");

void keepMQTTConnectionAlive(void *parameter)
{
    for (;;)
    {
        if (MQTTcli.connected())
        {
            MQTTcli.loop();
            vTaskDelay(500 / portTICK_PERIOD_MS);
            continue;
        }

        if (!WiFi.isConnected())
        {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        // Configure certificates
        // AWS_net.setCACert((const char *)aws_root_ca_pem_start);
        // AWS_net.setCertificate((const char *)certificate_pem_crt_start);
        // AWS_net.setPrivateKey((const char *)private_pem_key_start);

        serial_println(F("[MQTT] Connecting to AWS..."));
        serial_println(AWS_IOT_ENDPOINT);
        MQTTcli.begin(AWS_IOT_ENDPOINT, 1883, Wclient);

        long startAttemptTime = millis();
        MQTTcli.setHost(MQTT_ADDRESS, 1883);
        while (!MQTTcli.connect(DEVICE_NAME, MQTT_USER, MQTT_PASSWORD) &&
               millis() - startAttemptTime < MQTT_CONNECT_TIMEOUT)
        {
            vTaskDelay(MQTT_CONNECT_DELAY);
        }

        if (!MQTTcli.connected())
        {
            serial_println(F("[MQTT] AWS connection timeout. Retry in 30s."));
            vTaskDelay(30000 / portTICK_PERIOD_MS);
        }

        serial_println(F("[MQTT] AWS Connected!"));
    }
}

/**
 * TASK: Upload measurements to AWS. This only works when there are enough
 * local measurements. It's called by the measurement function.
 */
void uploadMeasurementsToMQTT(void *parameter)
{
    if (!WiFi.isConnected() || !MQTTcli.connected())
    {
        serial_println("[MQTT] AWS: no connection. Discarding data..");
        vTaskDelete(NULL);
    }

    char msg[AWS_MAX_MSG_SIZE_BYTES];
    strcpy(msg, "{\"readings\":[");

    for (short i = 0; i < LOCAL_MEASUREMENTS - 1; i++)
    {
        strcat(msg, String(measurements[i]).c_str());
        strcat(msg, ",");
    }

    strcat(msg, String(measurements[LOCAL_MEASUREMENTS - 1]).c_str());
    strcat(msg, "]}");

    serial_print("[MQTT] AWS publish: ");
    serial_println(msg);
    MQTTcli.publish(AWS_IOT_TOPIC, msg);

    // Task is done!
    vTaskDelete(NULL);
}


void sendEnergyToMQTT(void * parameter){
        if(!MQTTcli.connected()){
        serial_println("[MQTT] Can't send to HA without MQTT. Abort.");
        vTaskDelete(NULL);
        }

        char msg[30];
        strcpy(msg, "{\"power\":");
            strcat(msg, String(measurements[LOCAL_MEASUREMENTS-1]).c_str());
        strcat(msg, "}");

        serial_print("[MQTT] HA publish: ");
        serial_println(msg);

        MQTTcli.publish("homeassistant/sensor/" DEVICE_NAME "/state", msg);

        // Task is done!
        vTaskDelete(NULL);
    }


#endif
#endif
