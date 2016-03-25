#include <main.h>


#define START_BUTTON PIN_A4
/* --------------------------------------------- */






/* ---------- Timer control block -------------- */
void  RTCC_isr(void);
int16 delayTickCount = 0;
void Delay(int16 tick);
/* --------------------------------------------- */



/* ---------------- PWM block ------------------ */
#define leftPWMPeriod 200 //20ms
int8 leftPWMDutyCycle = 0;
int8 leftPWMTickCount = 0;
#define TurnOnLeftMotor() output_high(PIN_A1)
#define TurnOffLeftMotor() output_low(PIN_A1)
#define rightPWMPeriod 200 //20ms
int8 rightPWMDutyCycle = 0;
int8 rightPWMTickCount = 0;
#define TurnOnRightMotor() output_high(PIN_A0)
#define TurnOffRightMotor() output_low(PIN_A0)
/* --------------------------------------------- */



/* ---------- Scan sensor block ---------------- */
#define scanPeriod 200 //20ms
int8 scanTickCount = 0;
typedef struct SensorState
{
   int1 lef;
   int1 mid;
   int1 rig;
};
struct SensorState sensorState[4];
int8 currentSensorPosition = 0;
void scanSensor(void);
/* --------------------------------------------- */



/* ------------- Control block ----------------- */
#define enviroinmentScanRepeat 10 //20 scanner
int8 enviroinmentScanTickCount = 0;
int8 enviroinment;
int1 lastIsInWhiteSpace = 0;
#define transitionScanRepeat 50 //50 scanner
int8 transitionScanTickCount = 0;
int1 transitionTimerStart = 0;
int1 transitionState;
int1 isInWhiteSpace(void);
typedef enum
{
   GoAhead,
   TurnLeft,
   TurnRight
} CarState;
CarState check;
CarState lastCarState;  // luu gia tri lan cuoi
void CarTurnLeft_hard(void);
void CarTurnRight_hard(void);
void CarTurnLeft_medium(void);
void CarTurnRight_medium(void);
void CarGoAhead(void);
/* --------------------------------------------- */



/* ---------------- MAIN block ----------------- */
void main()
{ 
    //102 us overflow
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_2|RTCC_8_bit);      

   enable_interrupts(INT_RTCC);
   enable_interrupts(GLOBAL);
   
   while (input(START_BUTTON));
   delay_ms(500);
   lastCarState = GoAhead;
   while(TRUE)
   {
      switch ((sensorState[currentSensorPosition].lef<<3) | (sensorState[currentSensorPosition].mid<<2) | (sensorState[currentSensorPosition].rig<<1) | (isInWhiteSpace()))
      {
         case 0b0001: //all black in whitespace
         case 0b1110: //all white in blackspace
            break;    
        case 0b0000: //all black in blackspace
        case 0b1111: //all white in whitespace
            if(lastCarState==TurnRight) CarTurnRight_hard();
            else if(lastCarState==TurnLeft) CarTurnLeft_hard();
            else {
               if(check==TurnRight) CarTurnRight_hard();
               else if(check==TurnLeft) CarTurnLeft_hard();
               else CarGoAhead();
            }
            break;
         case 0b0100: //middle white in blackspace
         case 0b0101: //middle white in whitespace
         case 0b1010: //middle black in blackspace
         case 0b1011: //middle black in whitespace
            CarGoAhead();
            break;
         case 0b1000: //left white in blackspace
         case 0b0111: //left black in whitespace
            CarTurnLeft_hard();
            break;
         case 0b1001: //left white in whitespace
         case 0b0110: //left black in blackspace
            if (transitionState == 1)
               CarTurnLeft_medium();
            else
               CarTurnRight_medium();
            break;
         case 0b0010: //right white in blackspace
         case 0b1101: //right black in whitespace
            CarTurnRight_hard();
            break;
         case 0b0011: //right white in whitespace
         case 0b1100: //right black in blackspace
            if (transitionState == 1)
               CarTurnRight_medium();
            else
               CarTurnLeft_medium();
            break;
      }
   }
}
/* --------------------------------------------- */



/* ---------- Function implement block --------- */


#INT_RTCC
void  RTCC_isr(void)
{
   scanTickCount++;
   leftPWMTickCount++;
   rightPWMTickCount++;
   delayTickCount--;
   
   if (scanTickCount > scanPeriod)
   {
      scanSensor();
      scanTickCount = 0;
   } 
   
   /* Rotate Left Motor */
   if (leftPWMTickCount > leftPWMDutyCycle)
   {
      if (leftPWMTickCount > leftPWMPeriod)
      {
         leftPWMTickCount = 0;
         TurnOnLeftMotor();   
      }
      else 
      {
         TurnOffLeftMotor();
      }
   }
   
   /* Rotate Right Motor */
   if (rightPWMTickCount > rightPWMDutyCycle)
   {
      if (rightPWMTickCount > rightPWMPeriod)
      {
         rightPWMTickCount = 0;
         TurnOnRightMotor();       
      }
      else 
      {
         TurnOffRightMotor();
      }
   }
}

void scanSensor(void)
{
   int8 i;
   
   enviroinmentScanTickCount++;
   if (enviroinmentScanTickCount > enviroinmentScanRepeat)
   {
      currentSensorPosition++;
      if (currentSensorPosition > 3)
         currentSensorPosition = 0;
      enviroinmentScanTickCount = 0;
   }
   
   //get sensor value
   sensorState[currentSensorPosition].lef = input(PIN_B0);
   sensorState[currentSensorPosition].mid = input(PIN_B1);
   sensorState[currentSensorPosition].rig = input(PIN_B2);
   //show sensor state
   output_bit(PIN_B3, sensorState[currentSensorPosition].lef);
   //output_bit(PIN_B4, sensorState[currentSensorPosition].mid);
   //_bit(PIN_B5, sensorState[currentSensorPosition].rig);
   //output_bit(PIN_B3, lastCarState == TurnLeft);
   output_bit(PIN_B4, sensorState[currentSensorPosition].mid);
   //output_bit(PIN_B5, lastCarState == TurnRight);
   output_bit(PIN_B5, sensorState[currentSensorPosition].rig);
   //calc enviroinment
   enviroinment =  0;                
   for (i=0; i<4; i++)
   {
      if (sensorState[i].lef) enviroinment++;
      if (sensorState[i].mid) enviroinment++;
      if (sensorState[i].rig) enviroinment++;
   }
   
   if (isInWhiteSpace() != lastIsInWhiteSpace)
   {
      transitionTimerStart = 1;
      transitionState = 1;
      lastIsInWhiteSpace = isInWhiteSpace();
   }
   if (transitionTimerStart)
   {
      transitionScanTickCount++;
      if (transitionScanTickCount > transitionScanRepeat)
      {
         lastIsInWhiteSpace = isInWhiteSpace();
         transitionState = 0;
         transitionScanTickCount = 0;
         transitionTimerStart = 0;
      }
   }
}

int1 isInWhiteSpace(void)
{
   return (enviroinment > 6);
}

void CarTurnLeft_hard(void)
{
   leftPWMDutyCycle = 5;
   rightPWMDutyCycle = 200;
//   leftPWMDutyCycle = 0;
//   rightPWMDutyCycle = 50;
   check=lastCarState;
   lastCarState = TurnLeft;
   Delay(200);
}
void CarTurnRight_hard(void)
{
   leftPWMDutyCycle = 200;
   rightPWMDutyCycle = 5;
//   leftPWMDutyCycle = 50;
//   rightPWMDutyCycle =0 ;
   check=lastCarState;
   lastCarState = TurnRight;
   Delay(200);
}
void CarTurnLeft_medium(void)
{
  leftPWMDutyCycle = 15;
   rightPWMDutyCycle = 200;
//   leftPWMDutyCycle = 10;
//   rightPWMDutyCycle = 50;
   check=lastCarState;
   lastCarState = TurnLeft;
}
void CarTurnRight_medium(void)
{
   leftPWMDutyCycle = 200;
   rightPWMDutyCycle = 15;
//   leftPWMDutyCycle = 50;
//   rightPWMDutyCycle = 10;
   check=lastCarState;
   lastCarState = TurnRight;
}
void CarGoAhead(void)
{
   leftPWMDutyCycle =200;
   rightPWMDutyCycle = 200;
   check=lastCarState;
   lastCarState = GoAhead;
}

void Delay(int16 tick)
{
   delayTickCount = tick;
   while (delayTickCount != 0);
}
/* --------------------------------------------- */
