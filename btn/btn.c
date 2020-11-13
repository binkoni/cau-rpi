#include <wiringPi.h>
#include <stdio.h>
#define BTNPIN 3
void buttonPressed(void)
{
	printf("버튼이 눌렸습니다\n");
}

int main()
{
	wiringPiSetup();
	pinMode(BTNPIN, INPUT);
	wiringPiISR(BTNPIN, INT_EDGE_RISING, buttonPressed);
	while (1) {
		delay(100);
	}
	return 0;
}
