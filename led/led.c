#include <stdio.h>
#include <wiringPi.h>
int main(void)
{
	const int ledpin = 1;
	wiringPiSetup();
	pinMode(ledpin, OUTPUT);
	while (1) {
		digitalWrite(ledpin, HIGH);
		delay(500);
		digitalWrite(ledpin, LOW);
		delay(500);
	}
	return 0;
}
