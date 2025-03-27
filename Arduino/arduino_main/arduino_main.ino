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
    //mpu.CalibrateGyro(15);
    //mpu.CalibrateAccel(15);
    //Serial.println("\nCallibration complete");
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

    
    if (!isBroadcasting || !dmpReady) return;
    
    // Оптимизированное чтение FIFO
    if (mpu.getIntStatus() & 0x02) {
        mpu.getFIFOBytes(fifoBuffer, packetSize);
        
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);

        // Прямое чтение сырых данных без промежуточных переменных
        Serial.print(q.w, 4); Serial.print(',');
        Serial.print(q.x, 4); Serial.print(',');
        Serial.print(q.y, 4); Serial.print(',');
        Serial.print(q.z, 4); Serial.print(',');
        
        int16_t ax, ay, az;
        mpu.getAcceleration(&ax, &ay, &az);
        Serial.print(ax / ACCEL_CONST, 4); Serial.print(',');
        Serial.print(ay / ACCEL_CONST, 4); Serial.print(',');
        Serial.println(az / ACCEL_CONST, 4);
    }
}