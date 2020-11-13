#include <stdio.h>
#include <wiringPi.h>
#define BTNPIN 3

extern int capture_and_save_bmp();

void buttonPressed(void)
{
	printf("촬영을 시작합니다...\n");
	if (capture_and_save_bmp() < 0) {
		printf("촬영실패\n");
	} else {
		printf("촬영성공\n");
	}
}

int main()
{
	wiringPiSetup();
	pinMode(BTNPIN, INPUT);
	wiringPiISR(BTNPIN, INT_EDGE_RISING, buttonPressed);
	while(1) {
		delay(100);
	}
	return 0;
}
