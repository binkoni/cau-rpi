#include <stdlib.h>

extern int capture_and_save_bmp();

int main()
{
	if (capture_and_save_bmp() < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
