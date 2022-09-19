#ifndef TASK_MEASURE_ELECTRICITY
#define TASK_MEASURE_ELECTRICITY

#include <Arduino.h>
#include "EmonLib.h"

#include "../config/config.h"
#include "../config/enums.h"
#include "mqtt-aws.h"
#include "mqtt-home-assistant.h"

extern DisplayValues gDisplayValues;
extern EnergyMonitor emon1;
extern unsigned short measurements[];
extern unsigned char measureIndex;

void measureElectricity(void * parameter)
{
    for(;;){
      serial_println("[ENERGY] Measuring...");
      long start = millis();
    float L1realPower, L1apparentPower, L1powerFActor, L1supplyVoltage, L1Irms;
    // emon1.calcVI(20,2000);
    // L1realPower       = emon1.realPower;        //extract Real Power into variable
    // L1apparentPower   = emon1.apparentPower;    //extract Apparent Power into variable
    // L1powerFActor     = (emon1.powerFactor > 1) ? 1 :  (isnan(emon1.powerFactor) || isinf(emon1.powerFactor)) ? 0 :  emon1.powerFactor;      //extract Power Factor into Variable
    // L1supplyVoltage   = emon1.Vrms;             //extract Vrms into Variable
    // L1Irms            = emon1.Irms;             //extract Irms into Variable
    // serial_println("P:" + String(L1realPower) + " I: " + String(L1Irms));

    //   double amps = emon1.calcIrms(1480);
      double amps = emon1.calcIrms(1480);
      
      serial_println("I:" + String(amps)  );
      
      double watts = amps * HOME_VOLTAGE;

      gDisplayValues.amps = amps;
      gDisplayValues.watt = watts;

      measurements[measureIndex] = watts;
      measureIndex++;

      if(measureIndex == LOCAL_MEASUREMENTS){
          #if AWS_ENABLED == true
            xTaskCreate(
              uploadMeasurementsToAWS,
              "Upload measurements to AWS",
              10000,             // Stack size (bytes)
              NULL,             // Parameter
              5,                // Task priority
              NULL              // Task handle
            );
          #endif

          #if HA_ENABLED == true
            xTaskCreate(
              sendEnergyToHA,
              "HA-MQTT Upload",
              10000,             // Stack size (bytes)
              NULL,             // Parameter
              5,                // Task priority
              NULL              // Task handle
            );
          #endif

           #if MQTT_ENABLED == true
            xTaskCreate(
              sendEnergyToMQTT,
              "MQTT Upload",
              10000,             // Stack size (bytes)
              NULL,             // Parameter
              5,                // Task priority
              NULL              // Task handle
            );
          #endif

          measureIndex = 0;
      }

      long end = millis();

      // Schedule the task to run again in 1 second (while
      // taking into account how long measurement took)
      vTaskDelay((3000-(end-start)) / portTICK_PERIOD_MS);
    }    
}

#endif
