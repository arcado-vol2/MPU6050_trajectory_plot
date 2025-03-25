#include <Wire.h>
#include <MPU6050_6Axis_MotionApps20.h>

#define BUTTON_PIN 3
#define BUTTON_TIMER_AMOUNT 60

MPU6050 mpu;
bool dmpReady = false;
uint8_t mpuIntStatus;
uint8_t devStatus;
uint16_t packetSize;
uint16_t fifoCount;
uint8_t fifoBuffer[64];

Quaternion q;           // [w, x, y, z]
VectorFloat gravity;     // [x, y, z]

bool isBroadcasting = false;
bool isButtonPressed = false;
//can be removed using capacitor connected parallel to button
uint32_t buttonTimer = 0;

void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    Wire.begin();
    mpu.initialize();
    devStatus = mpu.dmpInitialize();
    
    if (devStatus == 0) {
        mpu.setDMPEnabled(true);
        dmpReady = true;
        packetSize = mpu.dmpGetFIFOPacketSize();
    }
    buttonTimer = millis();
}

void loop() {
    #pragma region Button_handler
    bool buttonStatus = !digitalRead(BUTTON_PIN);
    if (buttonStatus && !isButtonPressed && millis() - buttonTimer > BUTTON_TIMER_AMOUNT){
      isButtonPressed = true;
      buttonTimer = millis();
    }
    if (!buttonStatus && isButtonPressed && millis() - buttonTimer > BUTTON_TIMER_AMOUNT){
      isButtonPressed = false;
      buttonTimer = millis();
      
      isBroadcasting = !isBroadcasting;
      //i think we need a delay here cos use need some time to get his finger of the button
      if (isBroadcasting){
        Serial.println("Start recording");
        return;
      }
      else {
        Serial.println("Stop recording");
        delay(BUTTON_TIMER_AMOUNT);
        return;
      }
    }

    #pragma endringon Button_handler

    
    if (!isBroadcasting) return;
    if (!dmpReady) return;
    mpuIntStatus = mpu.getIntStatus();
    fifoCount = mpu.getFIFOCount();
    
    if ((mpuIntStatus & 0x10) || fifoCount == 1024) {
        mpu.resetFIFO();
    } 
    else if (mpuIntStatus & 0x02) {
        while (fifoCount < packetSize) 
          fifoCount = mpu.getFIFOCount(); //mb int better?
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        fifoCount -= packetSize;
        
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        
        Serial.print(q.w); Serial.print(",");
        Serial.print(q.x); Serial.print(",");
        Serial.print(q.y); Serial.print(",");
        Serial.println(q.z);
    }
}