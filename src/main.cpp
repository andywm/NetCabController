#include <Arduino.h>

//Pinout
constexpr int Pin_Motor_Direction 	= 12;
constexpr int Pin_Motor_Step 		= 11;
constexpr int Pin_Motor_Enable 		= 10;
constexpr int Pin_Switch_Drawer 	= 9;
constexpr int Pin_Switch_LimitMin 	= 8;
constexpr int Pin_Switch_LimitMax 	= 7;
constexpr int Pin_RaspPi_Power 		= 6;

constexpr int StepperPulseDuration  = 1000;

bool bShouldRaspberryPiBePowered = true;
bool bIsRaspberryPiPowered = true; //will power on automatically.
bool bIsProcessing = true;

#define GO_UP true
#define GO_DOWN false

//motor is active low. So define these for readability sake.
#define MOTOR_OFF true
#define MOTOR_ON false

void OnDrawerStateChanged_ISR()
{
	bIsProcessing = true;
}

void OnLimitHit_ISR()
{
	//kill motor power immediately.
	digitalWrite(Pin_Motor_Enable, MOTOR_OFF);
}

void setup()
{
	//Outputs
	pinMode(Pin_Motor_Direction, OUTPUT);
	pinMode(Pin_Motor_Step, OUTPUT);
	pinMode(Pin_Motor_Enable, OUTPUT);
	pinMode(Pin_RaspPi_Power, OUTPUT);

	//Interrupts
	attachInterrupt(Pin_Switch_LimitMin, OnLimitHit_ISR, RISING);
	attachInterrupt(Pin_Switch_LimitMin, OnLimitHit_ISR, RISING);
	attachInterrupt(Pin_Switch_Drawer, OnDrawerStateChanged_ISR, CHANGE);

	//Power on, initial state.
	OnDrawerStateChanged_ISR();
}

void HandleRaspberryPiPowerState()
{
	if (bShouldRaspberryPiBePowered == bIsRaspberryPiPowered)
	{
		return;
	}

	bIsRaspberryPiPowered = bShouldRaspberryPiBePowered;

	digitalWrite(Pin_RaspPi_Power, true);
	delay(500);
	digitalWrite(Pin_RaspPi_Power, false);
}

bool SeekMotorTowardsTarget()
{
	const bool bIsDrawerOpen = digitalRead(Pin_Switch_Drawer);
	bShouldRaspberryPiBePowered = bIsDrawerOpen;

	if (bIsDrawerOpen == true && digitalRead(Pin_Switch_LimitMax) == false)
	{
		//Raise
		digitalWrite(Pin_Motor_Direction, GO_UP);
		digitalWrite(Pin_Motor_Enable, MOTOR_ON);

		digitalWrite(Pin_Motor_Step, HIGH);
		delayMicroseconds(StepperPulseDuration);
		digitalWrite(Pin_Motor_Step, LOW);
		delayMicroseconds(StepperPulseDuration);
		return false;
	}
	else if (bIsDrawerOpen == false && digitalRead(Pin_Switch_LimitMin) == false)
	{
		//Retract
		digitalWrite(Pin_Motor_Direction, GO_DOWN);
		digitalWrite(Pin_Motor_Enable, MOTOR_ON);

		digitalWrite(Pin_Motor_Step, HIGH);
		delayMicroseconds(StepperPulseDuration);
		digitalWrite(Pin_Motor_Step, LOW);
		delayMicroseconds(StepperPulseDuration);
		return false;
	}

	digitalWrite(Pin_Motor_Enable, MOTOR_OFF);
	return true;
}

void loop() 
{
	if (!bIsProcessing)
	{
		return;
	}

	if (const bool bReachedTarget = SeekMotorTowardsTarget())
	{
		HandleRaspberryPiPowerState();
		bIsProcessing = false;
	}
}

