#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>                  
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <asm/types.h>              
#include <unistd.h>
#include <linux/videodev2.h>        

#include "bmpfile.h"


/* Pi Camera를 위한 디바이스 파일 */
#define VIDEODEV    "/dev/video0"               
/* 캡쳐받을 이미지 크기 */
#define IMGWIDTH       1024                              
#define IMGHEIGHT      768  
/* BMP 색상수 */
#define NUMCOLOR	  3

#define LIM(V,MIN,MAX) (V > MAX ? MAX : V < MIN ? MIN : V);

/* Video4Linux에서 사용할 영상 저장을 위한 버퍼 */
struct buffer {
	unsigned char *start;
	size_t length;
};

/* V4L2 장치 초기화 */
static unsigned int v4l2init(int fd)
{
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	unsigned int min;

	if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if(errno == EINVAL) {
			perror("/dev/video0 is no V4L2 device");
			return -1;
		} else {
			perror("VIDIOC_QUERYCAP");
			return -1;
		}
	}

	/* 장치가 영상 캡쳐 기능이 있는지 조사 */
	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		perror("/dev/video0 is no video capture device");
		return -1;
	}

	/* V4L2 드라이버에 캡처 파라미터 전달 (해상도, 포맷 등) */
	memset(&fmt, 0x00, sizeof (fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = IMGWIDTH;
	fmt.fmt.pix.height = IMGHEIGHT;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if(ioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
		perror("VIDIOC_S_FMT");
		return -1;
	}

	/* 이미지 크기는 카메라가 지원하는 최소 크기보다 커야함 */
	min = fmt.fmt.pix.width * 2;
	if(fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if(fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	/* 영상 읽기를 위한 초기화 */
	return fmt.fmt.pix.sizeimage;
}

/* BMP 이미지 데이터를 파일로 저장  */
void save_bmpimg(unsigned char *bmpimg)
{
	BITMAPFILEHEADER bmpheader;
	BITMAPINFOHEADER bmpinfoheader;
	RGBQUAD palrgb[256] = {0,};
	FILE *fp;

	memset(&bmpheader, 0x00, sizeof (bmpheader));
	bmpheader.bfType[0] = 'B';
	bmpheader.bfType[1] = 'M';
	bmpheader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfSize += sizeof(RGBQUAD) * 256;
	bmpheader.bfSize += IMGWIDTH*IMGHEIGHT*NUMCOLOR;
	bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmpheader.bfOffBits += sizeof(RGBQUAD) * 256; 

	memset(&bmpinfoheader, 0x00, sizeof (bmpinfoheader));
	bmpinfoheader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfoheader.biWidth = IMGWIDTH;
	bmpinfoheader.biHeight = IMGHEIGHT;
	bmpinfoheader.biPlanes = 1;
	bmpinfoheader.biBitCount = NUMCOLOR*8;
	bmpinfoheader.biSizeImage = IMGWIDTH*IMGHEIGHT*bmpinfoheader.biBitCount/8;
	bmpinfoheader.biXPixelsPerMeter = 0x0B12;
	bmpinfoheader.biYPixelsPerMeter = 0x0B12;

	/* capture.bmp 파일 생성 */
	if((fp = fopen("static/capture.bmp", "wb")) == NULL) {
		fprintf(stderr, "Error : Failed to open file...\n");
		exit(EXIT_FAILURE);
	}

	/* BMP 파일(BITMAPFILEHEADER) 정보 저장 */ 
	fwrite((void*)&bmpheader, sizeof(bmpheader), 1, fp);

	/* BMP 이미지(BITMAPINFOHEADER) 정보 저장 */ 
	fwrite((void*)&bmpinfoheader, sizeof(bmpinfoheader), 1, fp);

	/* RGBQUAD 팔레트 (256색) */
	fwrite(palrgb, sizeof(RGBQUAD), 256, fp);

	/* 실제 이미지 데이터 */
	fwrite(bmpimg, sizeof(unsigned char), NUMCOLOR * IMGWIDTH * IMGHEIGHT, fp);

	fclose(fp);
}

/* 카메라로부터 받아온 YUV422 이미지를 BMP로 저장 */
int yuv422_save_as_bmp(struct buffer* buffer) 
{
	int i, j;
	int y0, y1, u, v;
	int r, g, b;
	unsigned char *bmpimg;

	/* BMP 이미지를 위한 메모리 할당 */
	bmpimg = malloc(NUMCOLOR * IMGWIDTH * IMGHEIGHT);
	if (!bmpimg) {
		perror("malloc()");
		return -1;
	}

	/* YUV422(Y0 U Y1 V) 픽셀을 받아 RGB로 저장 */
	for (i=0,j=0; i<buffer->length; i+=4,j+=2) {
		y0 = buffer->start[i];
		u  = buffer->start[i+1];
		y1 = buffer->start[i+2];
		v  = buffer->start[i+3];

		/* 픽셀 1 */
		b = (float)y0 + (1.7790f * (float)(u - 128));
		g = (float)y0 + (0.3455f * (float)(u - 128) - (0.7169f * (float)(v - 128)));
		r = (float)y0 + (1.4075f * (float)(v - 128));
		bmpimg[j*3]		= LIM(b, 0, 255);
		bmpimg[j*3+1]	= LIM(g, 0, 255);
		bmpimg[j*3+2]	= LIM(r, 0, 255);

		/* 픽셀 2 */
		b = (float)y1 + (1.7790f * (float)(u - 128));
		g = (float)y1 + (0.3455f * (float)(u - 128) - (0.7169f * (float)(v - 128)));
		r = (float)y1 + (1.4075f * (float)(v - 128));
		bmpimg[j*3+3] = LIM(b, 0, 255);
		bmpimg[j*3+4] = LIM(g, 0, 255);
		bmpimg[j*3+5] = LIM(r, 0, 255);
	}
	save_bmpimg(bmpimg);
	free(bmpimg);
}

/* 카메라 장치로부터 이미지 하나를 읽어옴 */
int read_from_picam(int fd, struct buffer *buffer)
{
	while (1) {
		fd_set fds;
		struct timeval tv;
		int ret;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* 이벤트 대기 타임아웃 2초 */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		/* select 함수는 fd+1보다 작은 fd들로부터 발생하는 이벤트 감지 */
		ret = select(fd+1, &fds, NULL, NULL, &tv);
		switch(ret) {
			case -1:
				if (errno != EINTR) {
					perror("select()");
					return -1;
				}
				break;
			case 0: 
				perror("select()");
				return -1;
		}

		/* 읽기 성공시 루프 종료 */
		if (read(fd, buffer->start, buffer->length) < 0) {
			perror("read()");
			return -1;
		} else {
			break;
		}
	}

	return 0;
}

/* 외부에서 캡처 요청을 위해 사용하는 함수 */
extern int capture_and_save_bmp(void)
{
	int fd;
	int len;
	struct buffer buffer; 

	/* Pi Camera 열기 */
	fd = open("/dev/video0", O_RDWR| O_NONBLOCK, 0);
	if(fd == -1) {
		perror("open( ) : video devive");
		return -1;
	}

	/* Pi Camera 초기화 */
	len = v4l2init(fd);

	/* 버퍼 초기화 */
	buffer.start = malloc(len);
	if (!buffer.start) {
		perror("malloc()");
		return -1;
	}
	buffer.length = len;

	/* 카메라로부터 이미지 읽기 */
	if (read_from_picam(fd, &buffer) < 0) {
		perror("read_from_picam()");
		free(buffer.start);
		return -1;
	}

	/* BMP 형식으로 저장하기 */
	if (yuv422_save_as_bmp(&buffer) < 0) {
		perror("yuv422_save_as_bmp()");
		free(buffer.start);
		return -1;
	}

	free(buffer.start);
	close(fd);

	return 0;
}



