#include <wiringPi.h>
#include <stdio.h>

#define LEDPIN 1

int main()
{
	wiringPiSetup();
	pinMode(LEDPIN, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetClock(19);
	pwmSetRange(1000000);
	pwmWrite(LEDPIN, 1000000 / 2);
	return 0;
}
