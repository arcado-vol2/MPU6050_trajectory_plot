#include "MPU6050_6Axis_MotionApps20.h"

#define BUTTON_PIN 3
#define DEBOUNCE_DELAY 50
#define BUFFER_SIZE 45
#define SERIAL_SPEED 230400
#define ENABLE_CALIBRATION 1
#define ACCEL_SCALE 8192.0


MPU6050 mpu;
volatile bool mpuFlag = false;  // mpu interaption flag
volatile bool buttonPressed = false;
volatile bool buttonReleased = false; 
volatile bool outputEnabled = false; 
uint8_t fifoBuffer[BUFFER_SIZE];


unsigned long lastDebounceTime = 0; //just becouse i didn't have capacitor on button



void setup() {
  
  bool allowCalibration;

  if(digitalRead(8) == HIGH){
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
  Serial.print("Current accel range: ±");
  Serial.print(2 << mpu.getFullScaleAccelRange());
  Serial.println("g");

  mpu.dmpInitialize();
  mpu.setDMPEnabled(true);
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

void loop() {

  #pragma region button
  if (buttonReleased) {
    buttonReleased = false;
    buttonPressed = false;
    outputEnabled = !outputEnabled;
    if (outputEnabled) {
      Serial.println("Start recording");
    } else {
      Serial.println("Stop recording");
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
    
    mpuFlag = false;
    
    Serial.print(q.w);                Serial.print(',');
    Serial.print(q.x);                Serial.print(',');
    Serial.print(q.y);                Serial.print(',');
    Serial.print(q.z);                Serial.print(',');

    Serial.print(aa.x / ACCEL_SCALE); Serial.print(',');
    Serial.print(aa.y / ACCEL_SCALE); Serial.print(',');
    Serial.print(aa.z / ACCEL_SCALE); Serial.println();

  }
}