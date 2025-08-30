#include <Wire.h>
#include "Adafruit_VL6180X.h"
#include "vl53l4cd_class.h"

#define DEV_I2C Wire

// Adafruit_VL6180X v = Adafruit_VL6180X();
uint8_t xshutPin = 21;
VL53L4CD sensor_vl53l4cd_sat(&DEV_I2C, xshutPin);

void setup() {
  Serial.begin(115200);

  // wait for serial port to open on native usb devices
  while (!Serial) {
    delay(1);
  }

  Serial.println("Starting...");

  // Serial.println("Adafruit VL6180x test!");
  // if (! v.begin()) {
  //   Serial.println("Failed to find sensor");
  //   while (1);
  // }

  // Serial.println("Sensor found!");

  // Initialize I2C bus.
  DEV_I2C.begin();
  Serial.println("I2C bus initialized...");

  // Configure VL53L4CD satellite component.
  sensor_vl53l4cd_sat.begin();
  Serial.println("VL53L4CD configured...");

  // Switch off VL53L4CD satellite component.
  // sensor_vl53l4cd_sat.VL53L4CD_Off();

  //Initialize VL53L4CD satellite component.
  sensor_vl53l4cd_sat.InitSensor(21);
  Serial.println("VL53L4CD initialized...");

  // Program the highest possible TimingBudget, without enabling the
  // low power mode. This should give the best accuracy
  sensor_vl53l4cd_sat.VL53L4CD_SetRangeTiming(200, 50);
  Serial.println("Measurement timing set...");

  // Start Measurements
  sensor_vl53l4cd_sat.VL53L4CD_StartRanging();
  Serial.println("Measurement started...");
}
 

void loop() {
  // float lux = v.readLux(VL6180X_ALS_GAIN_5);

  // Serial.print("Lux: "); Serial.println(lux);

  // uint8_t range = v.readRange();
  // //uint8_t range1 = v2.readRange();
  // uint8_t status = v.readRangeStatus();
  // //uint8_t status1 = v2.readRangeStatus();

  // if (status == VL6180X_ERROR_NONE) {
  //   Serial.print("Range: "); Serial.println(range);
  // }

  // // Some error occurred, print it out!

  // if  ((status >= VL6180X_ERROR_SYSERR_1) && (status <= VL6180X_ERROR_SYSERR_5)) {
  //   Serial.println("System error");
  // }
  // else if (status == VL6180X_ERROR_ECEFAIL) {
  //   Serial.println("ECE failure");
  // }
  // else if (status == VL6180X_ERROR_NOCONVERGE) {
  //   Serial.println("No convergence");
  // }
  // else if (status == VL6180X_ERROR_RANGEIGNORE) {
  //   Serial.println("Ignoring range");
  // }
  // else if (status == VL6180X_ERROR_SNR) {
  //   Serial.println("Signal/Noise error");
  // }
  // else if (status == VL6180X_ERROR_RAWUFLOW) {
  //   Serial.println("Raw reading underflow");
  // }
  // else if (status == VL6180X_ERROR_RAWOFLOW) {
  //   Serial.println("Raw reading overflow");
  // }
  // else if (status == VL6180X_ERROR_RANGEUFLOW) {
  //   Serial.println("Range reading underflow");
  // }
  // else if (status == VL6180X_ERROR_RANGEOFLOW) {
  //   Serial.println("Range reading overflow");
  // }

  // delay(50);

  uint8_t NewDataReady = 0;
  VL53L4CD_Result_t results;
  uint8_t status;
  char report[64];

  Serial.println("Taking measurement...");

  do {
    status = sensor_vl53l4cd_sat.VL53L4CD_CheckForDataReady(&NewDataReady);
  } while (!NewDataReady);

  if ((!status) && (NewDataReady != 0)) {
    // (Mandatory) Clear HW interrupt to restart measurements
    sensor_vl53l4cd_sat.VL53L4CD_ClearInterrupt();

    // Read measured distance. RangeStatus = 0 means valid data
    sensor_vl53l4cd_sat.VL53L4CD_GetResult(&results);
    Serial.print("Status = ");
    Serial.println(results.range_status);
    Serial.print("Distance = ");
    Serial.println(results.distance_mm);
    Serial.print(report);
  }
}


/*GPIO23 -- original sensor
GPIO22 -- new  sensor

need to make the address (set up) -- reassign address ex) 0xnumber

loop to reassign the addresses */