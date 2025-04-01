#include "MPU6050_6Axis_MotionApps20.h"

#define BUTTON_PIN 3
#define DEBOUNCE_DELAY 50
#define BUFFER_SIZE 45
#define SERIAL_SPEED 115200
#define ENABLE_CALIBRATION 1
#define ACCEL_SCALE 8192.0
#define FILTER_COOEF 0.93

MPU6050 mpu;
volatile bool mpuFlag = false;  // mpu interaption flag
volatile bool buttonPressed = false;
volatile bool buttonReleased = false; 
volatile bool outputEnabled = false; 
uint8_t fifoBuffer[BUFFER_SIZE];


unsigned long lastDebounceTime = 0; //just becouse i didn't have capacitor on button
unsigned long lastMeasurementTime = 0; 


VectorInt16 aaFiltered = VectorInt16(0,0,ACCEL_SCALE);


void setup() {
  

  bool allowCalibration;

  if(digitalRead(8) == LOW){
    digitalWrite(A1, HIGH);
    allowCalibration = true;
  }
  else{
    digitalWrite(A1, LOW);
    allowCalibration = false;
  }

  Serial.begin(SERIAL_SPEED);

  Wire.begin();
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, CHANGE);

  mpu.initialize();
  mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
  //Serial.print("Current accel range: ±");
  //Serial.print(2 << mpu.getFullScaleAccelRange());
  //Serial.println("g");

  mpu.dmpInitialize();
  mpu.setDMPEnabled(true);
  mpu.setDLPFMode(MPU6050_DLPF_BW_20);
  attachInterrupt(0, dmpReady, RISING);

  
  #if ENABLE_CALIBRATION

  if (allowCalibration){
    mpu.CalibrateAccel(30);
    Serial.print("\nAccel calibration complete.\nAccel offsets: ");
    Serial.print(mpu.getXAccelOffset()); Serial.print(", ");
    Serial.print(mpu.getYAccelOffset()); Serial.print(", ");
    Serial.println(mpu.getZAccelOffset());

    mpu.CalibrateGyro(30);
    Serial.print("\nGyro calibration complete.\nGyro offsets: ");
    Serial.print(mpu.getXGyroOffset()); Serial.print(", ");
    Serial.print(mpu.getYGyroOffset()); Serial.print(", ");
    Serial.println(mpu.getZGyroOffset());
    digitalWrite(A1, LOW);
  }
  #endif
}

void dmpReady() {
  mpuFlag = true;
}

void buttonISR() {
  if ( (millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (digitalRead(BUTTON_PIN)) {
      buttonReleased = true;
    } else {
      buttonPressed = true; 
    }
  }
  lastDebounceTime = millis();
}

void CompFilter(int input, int& filtered){
  filtered = FILTER_COOEF * filtered + (1 - FILTER_COOEF) * input;
}

void loop() {

  #pragma region button
  if (buttonReleased) {
    buttonReleased = false;
    buttonPressed = false;
    outputEnabled = !outputEnabled;
    if (outputEnabled) {
      lastMeasurementTime = millis();
      Serial.println("1");
    } else {
      Serial.println("0");
    }
  }
  #pragma endregion
  
  if (outputEnabled && mpuFlag && mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {
    Quaternion q;
    VectorFloat gravity;
    VectorInt16 aa;
    float ypr[3];
    
    mpu.dmpGetQuaternion(&q, fifoBuffer);
    mpu.dmpGetGravity(&gravity, &q);
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
    mpu.dmpGetAccel(&aa, fifoBuffer);

    CompFilter(aa.x, aaFiltered.x);
    CompFilter(aa.y, aaFiltered.y);
    CompFilter(aa.z, aaFiltered.z);
    
    mpuFlag = false;
    
    Serial.print(millis() - lastMeasurementTime); Serial.print(',');
    Serial.print(q.w, 4);                Serial.print(',');
    Serial.print(q.x, 4);                Serial.print(',');
    Serial.print(q.y, 4);                Serial.print(',');
    Serial.print(q.z, 4);                Serial.print(',');

    Serial.print(aaFiltered.x / ACCEL_SCALE, 4); Serial.print(',');
    Serial.print(aaFiltered.y / ACCEL_SCALE, 4); Serial.print(',');
    Serial.print(aaFiltered.z / ACCEL_SCALE, 4); Serial.println();


    lastMeasurementTime = millis();

  }
}