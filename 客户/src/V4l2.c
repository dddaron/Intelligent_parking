#include<fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
#include<linux/videodev2.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include<string.h>
#include <linux/fb.h>
#include <errno.h>
#include <jerror.h>
#include<sys/select.h>
#include<sys/time.h>
#include<sys/types.h>
#include<stdio.h> 
/**************************************************************
操作v4l2的几个步骤：
1. 打开设备
	2. 查看设备功能
		3. 设置图片格式
			4. 申请帧缓冲
				5. 内存映射
					6. 帧缓冲入列
						7. 开始采集->读数据（包括处理数据）
					6. 帧缓冲重新入列
				5. 取消映射
			4. 释放帧缓冲
1. 关闭设备
***************************************************************/
struct buffer{  
	void *start;  
	unsigned int length;  
}*buffers; 
typedef struct Fream_Buffer
{
    unsigned char buf[1843200];
    int length;
}FreamBuffer;


#define DEV_PATH "/dev/video9" 
//#define DEV_PATH "/dev/usbdev1.6"


int takeCamPhoto(const char *cap_path)
{  
	int i=0;
	int ret;
	
	//1.打开摄像头设备 
	int fd = open(DEV_PATH,O_RDWR,0);							//以阻塞模式打开摄像头
	if(fd<0){
		printf("open device failed.\n");
	}
	
    //2.查询设备属性

	struct v4l2_capability cap;
	if(ioctl(fd,VIDIOC_QUERYCAP,&cap)==-1){
		printf("VIDIOC_QUERYCAP failed.\n");
	}
	//printf("\tDriverName:%s\n\tCardName:%s\n\tBusInfo:%s\n",cap.driver,cap.card,cap.bus_info);
//	printf("------------------------------------\n");


	//3.显示所有支持的格式
	struct v4l2_fmtdesc fmtdesc;
	fmtdesc.index = 0; 
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;					//视频捕捉模式
	while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) != -1){  
//		printf("\t%d.%s\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;
    }
//	printf("------------------------------------\n");

    //4.设置或查看当前格式
    struct v4l2_format fmt;
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 240;
	fmt.fmt.pix.height = 320;
    
	//常用格式有：V4L2_PIX_FMT_RGB32,   V4L2_PIX_FMT_YUYV   ,V4L2_STD_CAMERA_VGA  V4L2_PIX_FMT_JPEG
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	//fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;

	
	if (ioctl(fd,VIDIOC_S_FMT,&fmt) == -1) {
	   printf("VIDIOC_S_FMT failed.\n");
	   return -1;
    }

	if (ioctl(fd,VIDIOC_G_FMT,&fmt) == -1) {
	   printf("VIDIOC_G_FMT failed.\n");
	   return -1;
    }
//  	printf("\tfmt.fmt.width is %ld\n\tfmt.fmt.pix.height is %ld\n\tfmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);
//	printf("------------------------------------\n");
	
	//5.检查是否支持某种帧格式
	memset ( &fmt, 0, sizeof(fmt) );
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_MJPEG;
	
	if (ioctl(fd,VIDIOC_TRY_FMT,&fmt) == -1) {
	   printf("VIDIOC_TRY_FMT failed.\n");
	   return -1;
    }
	
//  	printf("\tVIDIOC_TRY_FMT sucess.\n\tfmt.fmt.width is %ld\n\tfmt.fmt.pix.height is %ld\n\tfmt.fmt.pix.colorspace is %ld\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.colorspace);
//	printf("------------------------------------\n");

	//6.1 申请拥有四个缓冲帧的缓冲区
	struct v4l2_requestbuffers req;  
    req.count = 4;								
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	if ( ioctl(fd,VIDIOC_REQBUFS,&req)==-1){  
		printf("VIDIOC_REQBUFS map failed.\n");  
		close(fd);  
		exit(0);  
	} 
//	printf("\tVIDIOC_REQBUFS map success.\n");
  
	//6.2 映射管理缓存区
	unsigned int n_buffers = 0;  
	buffers = calloc (req.count, sizeof(*buffers));					//将四个已申请到的缓冲帧映射到应用程序，用buffers 指针记录

	for(n_buffers = 0; n_buffers < req.count; ++n_buffers){  
		struct v4l2_buffer buf;  
		memset(&buf,0,sizeof(buf)); 
		buf.index = n_buffers;    //索引号
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
		buf.memory = V4L2_MEMORY_MMAP;  

		//查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
		if(ioctl(fd,VIDIOC_QUERYBUF,&buf) == -1){  
			printf("VIDIOC_QUERYBUF failed.\n");
			close(fd);  
			exit(-1);    
		} 
 //       printf("\tVIDIOC_QUERYBUF success.\n");

  		//帧缓存映射到用户空间
		buffers[n_buffers].length = buf.length;	
		buffers[n_buffers].start = mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buf.m.offset);  //映射到用户关键的
		if(MAP_FAILED == buffers[n_buffers].start){  
			printf("memory map failed.\n");
			close(fd);  
			exit(-1);  
		} 
//		printf("\tmemory map success.\n"); 
//		printf("\tVIDIOC_QBUF.->Frame buffer %d: address=0x%x, length=%d\n",n_buffers, (unsigned int)buffers[n_buffers].start, buffers[n_buffers].length);
	} 
//	printf("------------------------------------\n");
	
	//将帧缓冲添加到队列
	struct v4l2_buffer v4lbuf;
	memset(&v4lbuf,0,sizeof(v4lbuf));
	v4lbuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory=V4L2_MEMORY_MMAP;

	for(i=0;i<4;i++)
	{
		v4lbuf.index = i;
		ret = ioctl(fd , VIDIOC_QBUF, &v4lbuf);
		if (ret < 0) {
		printf("VIDIOC_DQBUF (%d) failed (%d)\n", i, ret);
			 return -1;
		}
	}
	
	//7.启动摄像头开始捕获数据
	enum v4l2_buf_type type; 
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 
	if (ioctl(fd,VIDIOC_STREAMON,&type) == -1) {
		printf("VIDIOC_STREAMON failed.\n");
		return -1;
	}
//	printf("\tVIDIOC_STREAMON success.\n"); 

//8.取出一帧
		fd_set fds;
		struct timeval tv;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		//int ret;
		ret = select(fd+1, &fds, NULL, NULL, &tv);
		//printf("------------------------------------\n");
	
	
		//pick one frame
		struct v4l2_buffer buf;
		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
	
		if(ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
		{
			printf("VIDIOC_DQBUF failed\n");
			return -10;
		}
//		printf("VIDIOC_DQBUF success\n");


	//9.处理当前帧数据
    FILE *fp = fopen(cap_path, "wb");
    if (fp < 0) {
        printf("open frame-data-file failed.\n");
        return -1;
    }
//	printf("\topen frame-data-file success.\n");
    fwrite(buffers[buf.index].start, 1, buf.length, fp);
    fclose(fp);
//    printf("\tsave one frame success.\n"); 
	
	//10.释放当前帧
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        printf("VIDIOC_QBUF failed.->fd=%d\n",fd);
        return -1;
    } 
//	printf("\tfree VIDIOC_QBUF success.\n");
	//11.关闭摄像头数据采集
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type))
	{
		printf ("Video VIDIOC_STREAMOFF failed !\n");
		return -1;
	}
	
	//12.释放映射缓冲区
    for (i=0; i< 4; i++) {
        munmap(buffers[i].start, buffers[i].length);
//		printf("munmap success.\n");
    }
   
	close(fd); 
//	printf("Camera test Done.\n"); 
	return 0;  
}  

