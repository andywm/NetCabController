#include <Arduino.h>
#include <ArduinoLowPower.h>

//Pinout
constexpr int Pin_Motor_Direction 	= 12;
constexpr int Pin_Motor_Step 		= 11;
constexpr int Pin_Motor_Enable 		= 10;
constexpr int Pin_Switch_Drawer 	= 9;
constexpr int Pin_Switch_LimitMin 	= 8;
constexpr int Pin_Switch_LimitMax 	= 7;
constexpr int Pin_RaspPi_Power 		= 6;

constexpr int StepperPulseDuration  = 10000;

bool bIsPiPowered = true; //initially will be on.
bool bPrevDrawerIsOpen = false;
bool bDrawerIsOpen = false;
bool bMotorDirection = false;
bool bActuateMotor = false;
bool bMotorDirectionChangeDetected = false;

#define GO_UP true
#define GO_DOWN false

bool PollIsKeyboardDrawerOpen()
{
	bPrevDrawerIsOpen = bDrawerIsOpen;
	bDrawerIsOpen = digitalRead(Pin_Switch_Drawer);
	return bDrawerIsOpen;
}

bool HasKeyboardDrawerStatusChanged()
{
	return bDrawerIsOpen != bPrevDrawerIsOpen;
}

bool IsMaxLimitTriggered()
{
	return digitalRead(Pin_Switch_LimitMax);
}

bool IsMinLimitTriggered()
{
	return digitalRead(Pin_Switch_LimitMin);
}

void OnDrawerStateChanged_ISR()
{
	bMotorDirectionChangeDetected = true;
}

void OnLimitHit_ISR()
{
	bActuateMotor = false;
	digitalWrite(Pin_Motor_Enable, LOW);
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
	LowPower.attachInterruptWakeup(Pin_Switch_Drawer, OnDrawerStateChanged_ISR, CHANGE);

	//Power on, initial state.
	OnDrawerStateChanged_ISR();
}

void HandleRaspberryPiPowerState()
{
	const bool bPiShouldBePowered = bMotorDirection == GO_UP;

	if (bPiShouldBePowered != bIsPiPowered)
	{
		digitalWrite(Pin_RaspPi_Power, true);
		delay(100);
		digitalWrite(Pin_RaspPi_Power, false);

		if (bPiShouldBePowered == false)
		{
			delay(1000);
			digitalWrite(Pin_RaspPi_Power, true);
			delay(100);
			digitalWrite(Pin_RaspPi_Power, false);
		}
	}
	bIsPiPowered = bPiShouldBePowered;
}

void HandleMotorModeChange()
{
	bMotorDirectionChangeDetected |= HasKeyboardDrawerStatusChanged();
	if (bMotorDirectionChangeDetected)
	{
		if (bDrawerIsOpen)
		{
			//Raise
			bActuateMotor = !IsMaxLimitTriggered();
			bMotorDirection = GO_UP;
		}
		else
		{
			//Retract
			bActuateMotor = !IsMinLimitTriggered();
			bMotorDirection = GO_DOWN;
		}

		if (bActuateMotor)
		{
			digitalWrite(Pin_Motor_Direction, bMotorDirection);
		}
		digitalWrite(Pin_Motor_Direction, bActuateMotor);

	}
	bMotorDirectionChangeDetected = false;
}

void loop() 
{
	PollIsKeyboardDrawerOpen();
	HandleMotorModeChange();

	if (bActuateMotor)
	{
		digitalWrite(Pin_Motor_Step, HIGH);
		delayMicroseconds(StepperPulseDuration);
		digitalWrite(Pin_Motor_Step, LOW);
		delayMicroseconds(StepperPulseDuration);
		return;
	}

	HandleRaspberryPiPowerState();
	LowPower.deepSleep();
}

