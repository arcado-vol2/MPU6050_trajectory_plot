#include <Wire.h>
#include <MPU6050_6Axis_MotionApps20.h>
#include <MPU6050.h>

#define BUTTON_PIN 3
#define BUTTON_TIMER_AMOUNT 60
#define ACCEL_CONST 16384.0
#define GYRO_CONST 131.0


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
    mpu.CalibrateGyro(15);
    mpu.CalibrateAccel(15);
    Serial.println("\nCallibration complete");
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

        int16_t ax, ay, az;  
        int16_t gx, gy, gz;  

        mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

        // Translate to G и deg/sec
        float accelX = ax / ACCEL_CONST;
        float accelY = ay / ACCEL_CONST;
        float accelZ = az / ACCEL_CONST;

        float gyroX = gx / GYRO_CONST;
        float gyroY = gy / GYRO_CONST;
        float gyroZ = gz / GYRO_CONST;

        Serial.print(q.w, 6); Serial.print(",");
        Serial.print(q.x, 6); Serial.print(",");
        Serial.print(q.y, 6); Serial.print(",");
        Serial.print(q.z, 6); Serial.print(",");
        Serial.print(accelX, 6); Serial.print(",");
        Serial.print(accelY, 6); Serial.print(",");
        Serial.println(accelZ, 6); Serial.print(",");
        Serial.print(gyroX, 6); Serial.print(",");
        Serial.print(gyroY, 6); Serial.print(",");
        Serial.println(gyroZ, 6);


    }
}