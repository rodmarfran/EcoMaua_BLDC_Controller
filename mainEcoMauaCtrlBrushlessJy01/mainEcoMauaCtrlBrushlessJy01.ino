#include "libAnalogCtrlIn.h"
#include "libJy01BrushlessCtrl.h"
#include "incBoardPins.h"
#include "incSystemConstants.h"

CAnalogCtrlIn xThrottle(CBoardPins::CU8_THROTTLE_AN_PIN, 700, 3100, 12);

CJy01BrushlessCtrl xJy01MotorCtrl(CBoardPins::CU8_JY01_CTRL_EL_DO_PIN, CBoardPins::CU8_JY01_CTRL_ZF_DO_PIN, CBoardPins::CU8_JY01_CTRL_M_DI_IRQ_PIN, 3.0f * CSystemConstants::CF_BLDC_MOTOR_POLE_PAIRS);

uint16_t u16ThrottleRaw = 0;
float fThrottlePercent = 0.0f;
bool bActivated = true;

void setup() {
  // put your setup code here, to run once:

  Serial.begin(9600);

  xThrottle.begin();

  xJy01MotorCtrl.begin(0x60);
  //xJy01MotorCtrl.setReverseDir();
  xJy01MotorCtrl.setFowardDir();

  /* tempo para o JY01 inicializar */
  delay(1000);

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(6, HIGH);
  digitalWrite(7, HIGH);

}

void loop() {
  // put your main code here, to run repeatedly:

  // u16ThrottleRaw = xThrottle.u16GetControlRaw();

  // if (u16ThrottleRaw == 0) {
  //   if (bActivated == true) {
  //     xJy01MotorCtrl.disableDrive();
  //     bActivated = false;
  //   }
  //   xJy01MotorCtrl.setControlRaw(0);
  // } else {
  //   if (bActivated == false) {
  //     xJy01MotorCtrl.enableDrive();
  //     bActivated = true;
  //   }
  //   xJy01MotorCtrl.setControlRaw(u16ThrottleRaw);
  // }

  // delay(100); 

  // read the sensor:
  if (Serial.available() > 0) {
    int inByte = Serial.read();
    // do something different depending on the character received.
    // The switch statement expects single number values for each case; in this
    // example, though, you're using single quotes to tell the controller to get
    // the ASCII value for the character. For example 'a' = 97, 'b' = 98,
    // and so forth:

    Serial.println((char)(inByte));

    switch (inByte) {
      case 'a':
        xJy01MotorCtrl.disableDrive();
        Serial.println("xJy01MotorCtrl.disableDrive()");
        break;
      case 'b':
        xJy01MotorCtrl.enableDrive();
        Serial.println("xJy01MotorCtrl.enableDrive()");
        break;
      case 'c':
        xJy01MotorCtrl.setControlPercent(0.0f);
        Serial.println("xJy01MotorCtrl.setControlPercent(0.0f)");
        break;
      case 'd':
        xJy01MotorCtrl.setControlPercent(0.5f);
        Serial.println("xJy01MotorCtrl.setControlPercent(0.5f)");
        break;
      case 'e':
        xJy01MotorCtrl.setControlPercent(1.0f);
        Serial.println("xJy01MotorCtrl.setControlPercent(1.0f)");
        break;
    }
    Serial.println("------");
  }

 delay(1000);

 Serial.println(xJy01MotorCtrl.xMEncoder.fGetSpeedRpm(1.0f));
 Serial.println(analogRead(A6));
 Serial.println(analogRead(A7));
 
 Serial.println("------");

}
