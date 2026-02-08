/* --- Includes --- */
// Wire.h for I2C communication with TCA chip and LiDAR
#include <Wire.h>
// Arduino standard encoder library
#include <Encoder.h>
// Adafruit library for VL6180X (short range) and VL53L0X (long ) LiDAR sensor
#include "Adafruit_VL6180X.h"
#include "Adafruit_VL53L0X.h"
// Adafruit library for VL53L0X (long range)
// Pin definitions
#include "micromouse_pins_2023.h"
// Robot Logic and Algorithm definitions
#include "Algo/FMicro.cpp"
/* ---- Defines ---- */
typedef enum motor_t {
  LEFT_MOTOR = 0,
  RIGHT_MOTOR = 1
} motor_t;

#define TEST
#undef log
#undef logf
#undef logln

#ifdef TEST
  #define log(...) Serial.print(__VA_ARGS__)
  #define logf(...) Serial.printf(__VA_ARGS__)
  #define logln(...) Serial.println(__VA_ARGS__)
  #define LOGGING 1
#else
  #define log(...)
  #define logf(...)
  #define logln(...)
  #define LOGGING 0
#endif

// Power deadband under which motors will not run
#define POWER_DEADBAND 0

#define LIDAR_COUNT 5
#define LIDAR_ADDR_BASE 0x50

// The physical distance between the sensors
// TODO: Check that values are consistent with new robot
#define LIDAR_SEPARATION_FB 40.0  // 39.9 mm between sensors front to back for Right Side
// #define LIDAR_SEPARATION_FB_L 40.6 // 40.6 mm between sensors front to back for Left Side the one above is right side
#define LIDAR_SEPARATION_LR 46.3  // 47 mm between sensors across robot
// boardwidth = 41.5

// Tolerance for turning function
int ANGLE_TOLERANCE = 20;
// Base speed for movement loop
int speed = 20;

// Encoder tick to rotation ratio
const double encoderTicks = 12;
// Motor gear ratio
const double gearRatio = 75;
// Distance between wheels in mm
const double wheelSeparation = 73.5;
// Radius of wheels in mm
const double wheelRadius = 17.2;
// Degree to encoder tick conversion ratio
const double turnRatio = (wheelSeparation / 2.0) / wheelRadius / 360 * gearRatio * encoderTicks;

// The LiDAR sensors return a running average of readings,
//  so when we move past a wall, the LiDAR returns a value greater than the previous value but less than an overflow.
// (After a certain amount of time, the running average overflows the maximum and only then does the LiDAR throw a read error)
// If the LiDAR is greater than this value, we assume that it's not sensing the wall.
#define SENSOR_RANGE_MAX 150
#define LONG_RANGE_SENSOR_RANGE_MAX 110 // TODO: Check whether this is accurate

// When centered, there should be ~60mm in front of the front sensor
#define LIDAR_FRONT_TARGET 90

// Squares are 10in by 10in, but we work in mm. 10in = 254 mm
#define SQUARE_SIZE 254

/* ---- User Variables ---- */

// GPIO pin numbers for the CS line on each LiDAR sensor
// TODO: Check inputs are correct
const int lidar_cs_pins[LIDAR_COUNT] = {LIDAR_FrontLeft, LIDAR_FrontRight, LIDAR_BackLeft, LIDAR_BackRight, LIDAR_FrontShort};

Adafruit_VL6180X lidar_sensors[LIDAR_COUNT];

// Flags to track out of range or otherwise errored sensors
bool front_left_errored, front_right_errored, back_left_errored, back_right_errored, forward_errored;
// Lidar sensor measurements
uint8_t forward;
uint8_t front_left;
uint8_t front_right;
uint8_t back_left;
uint8_t back_right;

Encoder rightEncoder (ENCODER_RIGHT_2, ENCODER_RIGHT_1);
Encoder leftEncoder (ENCODER_LEFT_1, ENCODER_LEFT_2);

/**
 * Convert a value in range [-127..127] to a motor power value
 *
 * @param p The input power [-127..127]
 * @return Output power [0..255]
 */
uint8_t convertPower(int8_t p) {
  if (p == 0) {
    return 255;
  }
  if (p < 0) {
    p = -p;
  }
  return 255 - (((uint8_t) p) * 2);
}

/**
 * Set motor power for a specified motor
 *
 * @param m The motor to modify
 * @param power The power and direction of the motor
 *              (range: [-127..127])
 *              Positive is "forward"
 *              Negative is "backward"
 */
void setMotor (motor_t m, int power) {
  // Ensure that power input is within bounds
  if (power < -127) {
    power = -127;
  }else if (power > 127) {
    power = 127;
  }
  int m1, m2;

  // Determine motor
  if (m == LEFT_MOTOR) {
    m1 = MOTORLEFT_1;
    m2 = MOTORLEFT_2;
  }else if (m == RIGHT_MOTOR) {
    m1 = MOTORRIGHT_2;
    m2 = MOTORRIGHT_1;
  }else {
    return;
  }
  // Set power
  if (power < POWER_DEADBAND && power > -POWER_DEADBAND) {
      // If power is within deadband, stop motors
      analogWrite(m1, 255);
      analogWrite(m2, 255);
  } else if (power < 0) {
      // If power is negative, run wheel backward
      analogWrite(m1, 255);
      analogWrite(m2, convertPower(power));
  } else {
      // If power is positive, run wheel forward
      analogWrite(m1, convertPower(power));
      analogWrite(m2, 255);
  }
}

/**
 * Detect whether left wall is present
 *
 * @return Boolean wall detection value
 */
int wallLeft() {
  // Read front left sensor distance
  front_left = lidar_sensors[0].readRange();
  logf("Left: %d\n", !(lidar_sensors[0].readRangeStatus() != VL6180X_ERROR_NONE || front_left > SENSOR_RANGE_MAX));
  // If sensor is not errored and within range, detect wall
  return !(lidar_sensors[0].readRangeStatus() != VL6180X_ERROR_NONE || front_left > SENSOR_RANGE_MAX);
}

/**
 * Detect whether right wall is present
 *
 * @return Boolean wall detection value
 */
int wallRight() {
  // Read front right sensor distance
  front_right = lidar_sensors[1].readRange();
  logf("Right: %d\n", !(lidar_sensors[1].readRangeStatus() != VL6180X_ERROR_NONE || front_right > SENSOR_RANGE_MAX));
  // If sensor is not errored and within range, detect wall
  return !(lidar_sensors[1].readRangeStatus() != VL6180X_ERROR_NONE || front_right > SENSOR_RANGE_MAX);
}

/**
 * Detect whether front wall is present
 *
 * @return Boolean wall detection value
 */
int wallFront() {
  // Read front sensor distance
  forward = lidar_sensors[4].readRange();
  logf("Front: %d\n", !(lidar_sensors[4].readRangeStatus() != VL6180X_ERROR_NONE || forward > SENSOR_RANGE_MAX));
  // If sensor is not errored and within range, detect wall
  return !(lidar_sensors[4].readRangeStatus() != VL6180X_ERROR_NONE || forward > SENSOR_RANGE_MAX);
}

/**
 * Update all sensors and error flags
 */
void updateSensors () {
  // Read the right LIDAR sensors and update their values (including error flags)
  back_right = lidar_sensors[1].readRange();
  back_right_errored = lidar_sensors[1].readRangeStatus() != VL6180X_ERROR_NONE || back_right > SENSOR_RANGE_MAX;
  front_right = lidar_sensors[3].readRange();
  front_right_errored = lidar_sensors[3].readRangeStatus() != VL6180X_ERROR_NONE || front_right > SENSOR_RANGE_MAX;

  // Read the left LIDAR sensors and update their values (including error flags)
  back_left = lidar_sensors[0].readRange();
  back_left_errored = lidar_sensors[0].readRangeStatus() != VL6180X_ERROR_NONE || back_left > SENSOR_RANGE_MAX;
  front_left = lidar_sensors[2].readRange();
  front_left_errored = lidar_sensors[2].readRangeStatus() != VL6180X_ERROR_NONE || front_left > SENSOR_RANGE_MAX;

  // Read the front short LIDAR sensor and update its value (including error flag)
  forward = lidar_sensors[4].readRange();
  forward_errored = lidar_sensors[4].readRangeStatus() != VL6180X_ERROR_NONE || forward > SENSOR_RANGE_MAX;
}

// p_controller(80.0, currentAngle, 0, -127.0, 127.0);
/**
 * Detect whether front wall is present
 *
 * @param p Output factor for p controller
 * @param current Current value for controller
 * @param goal Target value for controller
 * @param min Minimum output power
 * @param max Maximum output power
 * @return Output power [-127..127]
 */
double p_controller(double p, double current, double goal, double min, double max) {
  double out = (goal - current) * p;
  if (out > max) {
    out = max;
  }else if (out < min) {
    out = min;
  }
  return out;
}

/**
 * Get current robot angle from side LiDAR sensors
 * 
 * @return Angle calculated from LiDAR sensors [-pi..pi]
 *         Angle reported in radians
 *         Positive is counter-clockwise
 */
double getAngle()
{
  // arctan((lidarDistanceBL - lidarDistanceFL) / lidarSeparation);
  // Calculate robot angle from left and right side LiDAR sensors
  double leftAngle = -atan2(front_left - back_left, LIDAR_SEPARATION_FB);
  double rightAngle = atan2(front_right - back_right, LIDAR_SEPARATION_FB);
  // logf("left angle: %f\tright angle: %f; ", leftAngle * 180.0 / PI, rightAngle * 180.0 / PI);

  if ((back_left_errored || front_left_errored) && (back_right_errored || front_right_errored)) {
    // If we have no good data, assume we're going straight
    // logf("Using 0 as angle\n");
    return 0;
  } else if (back_left_errored || front_left_errored) {
    // If left sensors errored, use angle from right sensors
    // logf("Using right angle\n");
    return rightAngle;
  } else if (back_right_errored || front_right_errored) {
    // If right sensors errored, use angle from left sensors
    // logf("Using left angle\n");
    return leftAngle;
  } else {
    // If all sensors are not errored, average values
    // logf("Averaging angles\n");
    return (leftAngle + rightAngle) / 2;
  }
}

/**
 * Turn robot by a given angle in place
 *
 * @param angle     The angle to turn [-180..180]
 *                  Angle input in degrees
 * @param direction The direction to turn (LEFT or RIGHT)
 */
void turn(double angle, turning_direction_t direction) {
  // Encoder to turn
  Encoder *turnEncoder;
  Encoder *otherTurnEncoder;

  // Target encoder value
  double target = angle * turnRatio;

  // Direction constant
  int dir = 1;

  if (direction == LEFT)
  {
    // Turn left
    turnEncoder = &rightEncoder;
    otherTurnEncoder = &leftEncoder;
  } else {
    // Turn right
    turnEncoder = &leftEncoder;
    otherTurnEncoder = &rightEncoder;
    dir = -1;
  }

  // Zero out encoders
  turnEncoder->write(0);
  otherTurnEncoder->write(0);

  // Turn right wheel backwards if left, forwards if right
  // Scale by 0.7 to compensate for over-volted motors
  setMotor(RIGHT_MOTOR, 45.125 * dir * 0.7);
  // Turn left wheel forwards if left, backwards if right
  // Scale by 0.7 to compensate for over-volted motors
  setMotor(LEFT_MOTOR, -45.125 * dir * 0.7);

  // Turn until average encoder measurement is within margin of error
  int encoderAverage;
  do {
    encoderAverage = (turnEncoder->read() - otherTurnEncoder->read()) / 2;
    logf("%lu\n", turnEncoder->read());
    delay(1);
  } while (turnEncoder->read() < target - ANGLE_TOLERANCE);

  // Stop both motors
  setMotor(RIGHT_MOTOR, 0);
  setMotor(LEFT_MOTOR, 0);
}

/**
 * Turn robot by a given angle along a circular path
 *
 * @param angle     The angle to turn [-180..180]
 *                  Angle input in degrees
 * @param direction The direction to turn (LEFT or RIGHT)
 */
void movingTurn(double angle, turning_direction_t direction) {
  Encoder *turnEncoder;
  Encoder *otherTurnEncoder;

  // Modified degree to encoder tick conversion ratio for adjusted rotation path
  double turnRatio = (SQUARE_SIZE + wheelSeparation) / 2.0 / wheelRadius / 360 * gearRatio * encoderTicks;

  // Target encoder value
  double target = angle * turnRatio;

  // Faster wheel speed for outside of turning path
  double FAST_SPEED = 45.125 * (SQUARE_SIZE + wheelSeparation) / wheelSeparation * 0.35;
  // Slower wheel speed for inside of turning path
  double SLOW_SPEED = 45.125 * (SQUARE_SIZE - wheelSeparation) / wheelSeparation * 0.35;

  if(direction == LEFT){
    turnEncoder = &rightEncoder;
    otherTurnEncoder = &leftEncoder;
    // Correction for motor imbalance
    target = target * 10.5 / 12;
    // Zero out encoders
    turnEncoder->write(0);
    otherTurnEncoder->write(0);
    setMotor(RIGHT_MOTOR, FAST_SPEED);
    setMotor(LEFT_MOTOR, SLOW_SPEED);
  }
  else{
    turnEncoder = &leftEncoder;
    otherTurnEncoder = &rightEncoder;
    // Correction for motor imbalance
    target = target * 9 / 12;
    // Zero out encoders
    turnEncoder->write(0);
    otherTurnEncoder->write(0);
    setMotor(RIGHT_MOTOR, SLOW_SPEED);
    setMotor(LEFT_MOTOR, FAST_SPEED);
  }

  // Turn until average encoder measurement is within margin of error
  do {
    // wait
  } while (turnEncoder->read() < target - ANGLE_TOLERANCE);

  // Stop both motors
  setMotor(RIGHT_MOTOR, 0);
  setMotor(LEFT_MOTOR, 0);
}

/*
void movingTurn(double angle, turning_direction_t dir){
  Encoder *turnEncoder;
  Encoder *otherTurnEncoder;

  leftEncoder.write(0);
  rightEncoder.write(0);

  double ratio =  (SQUARE_SIZE/10 + wheelSeparation)/(SQUARE_SIZE/10 - wheelSeparation);
  double max = 50;
  int target = 1800;


  double FAST_SPEED = max*ratio/(ratio + 1);
  double SLOW_SPEED = max*1/(ratio + 1);
  
  if(dir == LEFT){
    turnEncoder = &rightEncoder;
    otherTurnEncoder = &leftEncoder;
    setMotor(RIGHT_MOTOR, FAST_SPEED);
    setMotor(LEFT_MOTOR, SLOW_SPEED);
  }
  else{
    turnEncoder = &leftEncoder;
    otherTurnEncoder = &rightEncoder;
    setMotor(RIGHT_MOTOR, SLOW_SPEED);
    setMotor(LEFT_MOTOR, FAST_SPEED);
  }
  int encoderAverage = 0;
    do {
      encoderAverage = (turnEncoder->read() - otherTurnEncoder->read()) / 2;
    } while (encoderAverage < target);

  setMotor(RIGHT_MOTOR, 0);
  setMotor(LEFT_MOTOR, 0);
}*/

/**
 * Rotate 90 degrees right
 */
void turnRight(){
  logln("Turning Right");
  turn(90.0 + getAngle() * 180.0 / PI, RIGHT);
}

/**
 * Rotate 90 degrees left
 */
void turnLeft(){
  logln("Turning Left");
  turn(90.0 - getAngle() * 180.0 / PI, LEFT);
}

/**
 * Rotate 90 degrees right while moving forward
 */
void movingTurnRight(){
  logln("Moving Turn Right");
  movingTurn(90.0 + getAngle() * 180.0 / PI, RIGHT);
}

/**
 * Rotate 90 degrees left while moving forward
 */
void movingTurnLeft(){
  logln("Moving Turn Left");
  movingTurn(90.0 - getAngle() * 180.0 / PI, LEFT);
}

/**
 * Rotate 45 degrees right
 */
void turnRight45(){
  turn(45.0 + getAngle() * 180.0 / PI, RIGHT);
}

/**
 * Rotate 45 degrees left
 */
void turnLeft45(){
  turn(45.0 - getAngle() * 180.0 / PI, LEFT);
}

/**
 * Rotate 180 degrees
 */
void turn180(){
  turn(180.0 - getAngle() * 180.0 / PI, LEFT);
}

/**
 * Move forward a specified number of squares
 * 
 * @param number The number of squares to move
 */
int moveForward(int number) {
  // Reset encoders
  leftEncoder.write(0);
  rightEncoder.write(0);

  // Create angle variables
  double currentAngle, angularVelocity;

  // Track current position
  double currentDistance = 0;
  // Set goal distance as a multiple of the square size
  double goalDistance = SQUARE_SIZE * number;
  // Create speed variable
  double velocity;

  // Center variables
  double centerVelocity, centerOffset;

  // Loop until goal distance is reached
  while (1) {
    // Update currentDistance and currentAngle
    {
      // Update LiDAR sensor readings
      updateSensors();
      // Update angle
      currentAngle = getAngle();

      // Update encoder readings
      long leftRevs = leftEncoder.read();
      long rightRevs = rightEncoder.read();
      // Update current distance
      // ((Num ticks of both wheels / 2) / num ticks per revolution) * PI * wheel diameter
      currentDistance = (((leftRevs + rightRevs) / 2.0) / (gearRatio * encoderTicks)) * PI * 2 * wheelRadius;
      logf("Current Dist:\t%f\n", currentDistance);
    }

    // logf("Moving forward. current: %d, ultra: %d, goal: %d, cond: %d, %d\n", currentDistance, ultrasonic, goalDistance, ultrasonic < 150, ultrasonic > 95);

    // Prevent breaking out of the loop if farther than 95 mm from a wall
    // If robot has reached internal goal distance, increment goal distance to go farther
    if (currentDistance >= goalDistance && forward < 150 && forward > 95) {
      logf("Moving goalDistance forward.\n");
      // Increase goal distance such that the forward ends up (60mm) away from the wall in front of us
      goalDistance += forward - LIDAR_FRONT_TARGET;
      redLights();
    }

    // Check if currentDistance and currentAngle are within tolerance of target, breaking out of loop if so
    // For the lidar, 60 is 60 mm from the wall (approximately centered in the square)
    if (currentDistance >= goalDistance || (!forward_errored && forward < LIDAR_FRONT_TARGET)) {
      setMotor(LEFT_MOTOR, 0);
      setMotor(RIGHT_MOTOR, 0);
      logf("Stopped. Long Range Lidar: %d, %d, %lf\n", !forward_errored, forward < LIDAR_FRONT_TARGET, forward);
      if(digitalRead(RED_LED) == HIGH)
        digitalWrite(RED_LED, LOW);
      greenLights();
      greenLights();
      break;
    }
 
    // How far away from the center we are
    // Right is positive
    // (ASCII art by Zach)
    // |              | <--  MAZE    |
    // |              |   CENTERLINE |
    // |              |              |
    // |      ERROR   |              |
    // |           \  |              |
    // |           |<>|              |
    // |       +-------+ <--- X ---> |
    // |       | FRONT |             |
    // |       |L  |  R|             |
    // |       |   |   |             |
    // |       | ROBOT |             |
    // |       +-------+             |
    // |       |<- R ->|             |
    // |              |              |
    // |              |              |
    // |              |              |
    // |              |              |
    // |              |              |
    // | <------ MAZE WIDTH -------> |
    //
    //              MAZE WIDTH - R
    // ERROR = X - ----------------
    //                    2
    // Compute offset as difference between left and right side sensors
    // TODO: Check whether this should be divided by 2
    centerOffset = (double)front_right - (double)front_left;
    if (front_left_errored && front_right_errored) {
        // If both sensors are errored, assume centered
        // logf("both errored, setting offset to 0\n");
        centerOffset = 0;
    }else if (front_left_errored) {
        // If we don't have a left value, compute offset from right sensor value only
        // (We're targeting to an offset of 0)
        // sensors are 84 mm apart, maze is 240mm wide
        centerOffset = (double)front_right - (240 - LIDAR_SEPARATION_LR) / 2.0;
    }else if (front_right_errored) {
        // If we don't have a right value, compute offset from left sensor value only
        centerOffset = (240 - LIDAR_SEPARATION_LR) / 2.0 - (double)front_left;
    }

    // logf("left %d; right %d: center offset: %f\n", front_left, front_right, centerOffset);

    // Compute correction for robot angle
    // When the angle is 0.1, we need to bump left power by like 5, so P of 20
    // (A positive angle means that we're turned left)
    angularVelocity = p_controller(40.0, currentAngle, 0, -127.0, 127.0);

    // Compute base velocity for forward movement
    // With a distance of 254 (one square), we've chose a P of 12.25
    // so it saturates velocity for the majority of the distance
    velocity = p_controller(6, currentDistance, goalDistance, -speed, speed);

    // Compute correction for horizontal displacement from centerline
    // With a center off set of 10mm, that's a velocity of 5
    centerVelocity = p_controller(0.20, centerOffset, 0, -50, 50);

    // Sume correction velocities
    angularVelocity += centerVelocity;

    // NOTE: We're assuming that at angles close to 0, angularVelocity has a linear relationship with velocity.

    // Update motor speeds
    // Scale by 0.7 to compensate for over-volted motors
    int velocityLeft = (int)(-angularVelocity / 2.0 * 0.7) + velocity;
    int velocityRight = (int)(angularVelocity / 2.0 * 0.7) + velocity;

    // logf("Left: %d\n", velocityLeft);
    // logf("Right: %d\n", velocityRight);
    // logf("Angular: %f\n", angularVelocity);
    // logf("centerVelocity: %f\n", centerVelocity);
    // logf("velocity: %f\n", velocity);
    // logf("Front: %d\n", forward);
    // logf("FLeft: %d\n", front_left);
    // logf("FRight: %d\n", front_right);
    // logf("BLeft: %d\n", back_left);
    // logf("BRight: %d\n", back_right);
    // logf("Angle: %f\n", getAngle());
    setMotor(LEFT_MOTOR, velocityLeft);
    setMotor(RIGHT_MOTOR, velocityRight);
  }
  return 0;
}

/* ---- SETUP ---- */
void setup() {
  // Start serial
  Serial.begin(115200);
  logln("Serial ready!");

  // Debug led on the board itself
  pinMode(DEBUG_LED, OUTPUT);

  pinMode(YELLOW_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  logln("Lights ready!");

  digitalWrite(DEBUG_LED, HIGH);

  // Starts I2C on the default pins (18 (SDA), 19 (SCL))
  // (I think, I can't find docs on it)
  I2C_LIDAR.begin();
  logln("I2C ready!");

  // Setup LiDARs
  // short range lidars
  for (size_t i = 0; i < LIDAR_COUNT; i++) {
    pinMode(lidar_cs_pins[i], OUTPUT);
  }
  // Disable all sensors except the first
  for (size_t i = 0; i < LIDAR_COUNT; i++) {
    digitalWrite(lidar_cs_pins[i], LOW);
  }

  // Set address for each sensor
  // Write the CS line high (turning it on)
  // Set the address
  for (size_t i = 0; i < LIDAR_COUNT; ++i) {
    digitalWrite(lidar_cs_pins[i], HIGH);
    // Pass pointer to the Wire2 object since we're running on I2C bus 2
    if (!lidar_sensors[i].begin(&I2C_LIDAR)) {
      log("Failed init on sensor ");
      logln(i);
    } else {
      lidar_sensors[i].setAddress(LIDAR_ADDR_BASE + i);
      log("Succeeded init on sensor ");
      logln(i);
    }
  }

  logln("LiDAR sensors ready!");

  // Setup motors
  pinMode(MOTORLEFT_1, OUTPUT);
  pinMode(MOTORLEFT_2, OUTPUT);
  pinMode(MOTORRIGHT_1, OUTPUT);
  pinMode(MOTORRIGHT_2, OUTPUT);

  // setMotor(RIGHT_MOTOR, 0);
  // setMotor(LEFT_MOTOR, 0);

  logln("Motors ready!");

  pinMode(START_BUTTON, INPUT);
  int t = 0;
  // Spin until start button is pressed
  // t is ms
  // On for 300 (0-300) off for 500 (300-800)
  while(!digitalRead(START_BUTTON)) {
    if (t == 0) {
      digitalWrite(YELLOW_LED, HIGH);
    }else if (t == 300) {
      digitalWrite(YELLOW_LED, LOW);
    }

    t = (t + 10) % 800;
    delay(10);
  }
  digitalWrite(YELLOW_LED, LOW);

  delay(20); // Switch "debounce"
  
  digitalWrite(LED0, HIGH);
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  long pressTime = millis();
  long endPressTime = pressTime;
  while (digitalRead(START_BUTTON)) {
    /* spin, waiting for button release */
    endPressTime = millis();
    if (endPressTime - pressTime > 1000) {
      speed = 25;
      digitalWrite(LED2, LOW);
    }
    if (endPressTime - pressTime > 3000) {
      speed = 40;
      digitalWrite(LED1, LOW);
    }
  } 
  if (endPressTime - pressTime < 1000) {
  }else if (endPressTime - pressTime < 3000) {
  }else {
  }
  delay(500);
  digitalWrite(LED0, LOW);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  initialize();
}

void coolLights(){
  if(digitalRead(BLUE_LED) == LOW)
    digitalWrite(BLUE_LED, HIGH);
  else
    digitalWrite(BLUE_LED, LOW);
    delay(500);
}

void redLights(){
  if(digitalRead(RED_LED) == LOW)
    digitalWrite(RED_LED, HIGH);
  else
    digitalWrite(RED_LED, LOW);
    delay(10);
}

void greenLights(){
  if(digitalRead(GREEN_LED) == LOW)
    digitalWrite(GREEN_LED, HIGH);
  else
    digitalWrite(GREEN_LED, LOW);
    delay(50);
}

void readLidar() {
}


float maxVoltage = 7.4;
float getOperatingVoltage()
{
  return analogRead(23)/1023*3;
}
int getMotorAdjustment(int analogValue)
{
  float value = (maxVoltage/getOperatingVoltage())*analogValue;
  if (value >= 255)
    return 255;
  return (int)(value);
}
/* ---- MAIN ---- */
void loop() {
  updateSensors();
  doRun();
}