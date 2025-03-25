
#### Code was written for Arduino Nano
For compile u need to install mpu 6050 libs

This code assume that u will use standart mpu 6050 connection:

+ VCC -> +5V
+ GND -> GND
+ SCL -> SCL (A5; Arduino Nano pinout)
+ SDA -> SDA (A4; Arduino Nano pinout)

Also code utilize button on ~D3 pin, u can change it in code by changing BUTTON_PIN define


--- 

#### Код был написан для Arduino Nano
Чтобы всё скомпилировалось надо поставить библиотеки для mpu 6050

Код предполагает стандартное подключение для mpu 6050:

+ VCC -> +5V
+ GND -> GND
+ SCL -> SCL (A5; Arduino Nano распиновка)
+ SDA -> SDA (A4; Arduino Nano распиновка)

Так же в нём есть кнопка, но её пин можно переопределить через BUTTON_PIN
