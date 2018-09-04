//-------------------------------------------------------------
/**
\file      GxTriggerSoftwareAcquire.cpp
\brief     sample to show how to acquire image continuously. 
\version   1.0.1709.9181
\date      2017-09-18
*/
//-------------------------------------------------------------

#include "GxIAPI.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>


#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;


#define MEMORY_ALLOT_ERROR -1 


Mat test(1024,1280,CV_8U);   //OpenCV Mat
GX_FRAME_DATA *g_frame_data={0};      
GX_DEV_HANDLE hDevice = NULL;
pthread_t g_acquire_thread = 0;             ///< The ID of acquisition thread
bool g_get_image = false;                   ///< The flag of acquisition thread



size_t   iSize = 0;


//Get the image size and allocate the memory.
int PreForImage();

//Release the resources
int UnPreForImage();

//The acquisition thread
void *ProcGetImage(void* param);

int main()
{
/*    uid_t user = 0;

    user = geteuid();
    if(user != 0)
    {
        printf("\n");  
        printf("Please run this application with 'sudo -E ./GxTestAcquire or"
                              " Start with root !\n");
        printf("\n");
        return 0;
    }
*/
    printf("\n");
    printf("-------------------------------------------------------------\n");
    printf("sample to show how to acquire image continuously.\n");
    printf("version: 1.0.1709.9181\n");
    printf("-------------------------------------------------------------\n");
    printf("\n");

    printf("Initializing......"); 
    printf("\n\n");
     
    //API接口函数返回值 
    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t ui32FrameCount = 0;
    uint32_t ui32AcqFrameRate = 0;
    uint32_t device_num = 0;
    uint8_t  ui8DevInfo[64] = {0};


    uint32_t ret=0;
 
    //初始化库
    status = GXInitLib(); 
    if(status != GX_STATUS_SUCCESS)
    {
        return 0;
    }
       
    printf("GXUpdateDeviceList(&device_num, 1000)\n");
    //更新设备列表
    status = GXUpdateDeviceList(&device_num, 1000);

    //获取枚举设备个数
    if(status != GX_STATUS_SUCCESS)
    { 
        status = GXCloseLib();
        return 0;
    }

    if(device_num <= 0)
    {
        printf("<No device>\n");
        status = GXCloseLib();
        return 0;
    }
    else
    {
        printf("DeviceNum is: %d\n", device_num);
    }
    
    //打开第一个设备	
    status = GXOpenDeviceByIndex(0, &hDevice);
    printf("Open Device status = %d\n",status);


    //设置采集队列Buffer个数
    uint64_t nBufferNum = 5;
    GXSetAcqusitionBufferNumber(hDevice, nBufferNum);
    printf("1  here......\n"); 



    //设置URB大小和个数
    GXSetInt(hDevice, GX_DS_INT_STREAM_TRANSFER_SIZE, (64*1024));
    GXSetInt(hDevice, GX_DS_INT_STREAM_TRANSFER_NUMBER_URB, 64);   

    // set trigger Mode  0:one frame    1:multi-frame    2:continuous
    int32_t i32SetTriggerMode = 2;
    iSize = sizeof(int32_t); 
    status = GXWriteRemoteDevicePort(hDevice, 0x00900020, &i32SetTriggerMode, &iSize);

    printf("2  here......\n"); 

    // Set Trigger Source
    int32_t i32SetTriggerSource = 0;
    iSize = sizeof(int32_t);
    status = GXWriteRemoteDevicePort(hDevice, 0x00900024, &i32SetTriggerSource, &iSize);
   
    // set ThroughputLimitMode
    int32_t i32ThroughputLimitMode = 0;
    iSize = sizeof(int32_t);
    status = GXWriteRemoteDevicePort(hDevice, 0x009000D0, &i32ThroughputLimitMode, &iSize);    
    
    // set ThroughputLimitValue
    int32_t i32ThroughputLimitVal = 400000000;
    iSize = sizeof(int32_t);
    status = GXWriteRemoteDevicePort(hDevice, 0x009000D4, &i32ThroughputLimitVal, &iSize);

    // 设置帧率控制开关，1为开启，如果不开启，U3V相机接在U2口上可能会丢帧
    int32_t i32ControlFrameRate = 1;
    iSize = sizeof(int32_t); 
    status = GXWriteRemoteDevicePort(hDevice, 0x009000E8, &i32ControlFrameRate, &iSize);	
        
    // 设置帧率值，设置值=帧率×10，例如设置值为190，相机输出帧率为19.0fps
    int32_t i32FrameRate = 900;
    iSize = sizeof(int32_t); 
    status = GXWriteRemoteDevicePort(hDevice, 0x009000EC, &i32FrameRate, &iSize);

    // 设置曝光,单位是us
    int32_t i32ExposureTime = 3000;
    iSize = sizeof(int32_t); 
    status = GXWriteRemoteDevicePort(hDevice, 0x00900038, &i32ExposureTime, &iSize);
    if(status != GX_STATUS_SUCCESS)
    {
	printf("Set Exposure time Failed ret= %d\n",status);
    } 

    printf("3  here......\n"); 


    //Prepare for image acquisition
    ret = PreForImage();
    if(ret != 0)    
    {
        printf("<Failed to prepare for image acquisition >\n");
        status = GXCloseDevice(hDevice);
        if(hDevice != NULL)
        {
            hDevice = NULL;
        }
        status = GXCloseLib();
        return 0;
    }

    printf("4  here......\n"); 

    //Start the acquisition thread
    ret = pthread_create(&g_acquire_thread, 0, ProcGetImage, 0);
    if(ret != 0)
    {
        printf("<Failed to create the collection thread>\n");
        status = GXCloseDevice(hDevice);
        if(hDevice != NULL)
        {
            hDevice = NULL;
        }
        status = GXCloseLib();
        return 0;
    }

    printf("5  here......\n"); 

    bool run = true;
    while(run == true)
    {
        int c = getchar();

        switch(c)
        {
            //exit
            case 'X': 
            case 'x':
                run = false;
                break;
            default:
                break;
        }	

    }


    printf("6  here......\n"); 
    //Prepare to stop image acquisition
    ret = UnPreForImage();
    if(ret != 0)
    {
        status = GXCloseDevice(hDevice);
        if(hDevice != NULL)
        {
            hDevice = NULL;
        }
        status = GXCloseLib();
        return 0;
    }
    
    //关闭设备
    status = GXCloseDevice(hDevice);
    if (status != 0)
    {
    	printf("GXCloseDevice failed&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    }
	   
    //释放库
    status = GXCloseLib();
    if (status != 0)
    {
    	printf("GXCloseLib failed&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    }
    printf("App Exit!\n");
    return 0;
}

//-------------------------------------------------
/**
\Get the image size and allocate the memory.
\return int
*/
//-------------------------------------------------
int PreForImage()
{
    GX_STATUS status = GX_STATUS_SUCCESS;

    //开始采集
    status = GxStreamOn(hDevice);
    if (status != GX_STATUS_SUCCESS)
    {
 	printf("AcqusitionStart Failed ret= %d\n",status);
    }	
 
 	int32_t i32TriggerSoftware = 1;
        iSize = sizeof(int32_t); 
        status = GXWriteRemoteDevicePort(hDevice, 0x00800008, &i32TriggerSoftware, &iSize);

	return 0;
}

//-------------------------------------------------
/**
\ Release the resources
\return int
*/
//-------------------------------------------------
int UnPreForImage()
{
    GX_STATUS status = GX_STATUS_SUCCESS;
    uint32_t ret = 0;
   
    //停止采集
    status = GxStreamOff(hDevice);
    if (status != 0)
    {
    	printf("GxStreamOff failed&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
    }

    g_get_image = false;
    ret = pthread_join(g_acquire_thread,NULL);
    if(ret != 0)
    {
        printf("<Failed to release resources>\n");
        return ret;
    }
	
    //Free the buffer
    //if(g_frame_data->pImgBuf != NULL)
    //{
        //free(g_frame_data->pImgBuf);
        //g_frame_data->pImgBuf = NULL;
    //}

    return 0;
}

//-------------------------------------------------
/**
\brief  The function of acquisition thread
\param   pParam 
\return  void*
*/
//-------------------------------------------------
void *ProcGetImage(void* pParam)
{
    printf("In ProcGetImage\n");

    GX_STATUS status = GX_STATUS_SUCCESS;
    g_get_image = true;
	
    while(g_get_image)
    {
        //if(g_frame_data->pImgBuf == NULL)
        //{
	  // printf("g_frame_data->pImgBuf == NULL\n");
          //  continue;
       // }

        status = GxDQBuf(hDevice, &g_frame_data, 1000);
        if(g_frame_data != NULL)
        { 

	   printf("Transforming to Mat\n");

	   memcpy(test.data,g_frame_data->pImgBuf,g_frame_data->nImgSize);

	   imshow("test",test);

    	   if (waitKey(30)=='q')
           {
             destroyWindow("test");
             pthread_exit(0);
           }	
	    //将图像buffer放回库中
	   GxQBuf(hDevice,g_frame_data);
        }
        else
        {
            printf("GxDQBuf failed&&&&&&&&&&&&&&&&&&&&&&&&&&& %d\n", status);
        } 
    }
}





