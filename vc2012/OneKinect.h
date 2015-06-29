#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;
using namespace std;

//KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK
#include "windows.h"
#include "NuiApi.h"
//KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK

static const Scalar SKELETON_COLORS[NUI_SKELETON_COUNT] =
{
	Scalar(0, 0, 255),      // Blue
	Scalar(0, 255, 0),      // Green
	Scalar(255, 255, 64),   // Yellow
	Scalar(64, 255, 255),   // Light blue
	Scalar(255, 64, 255),   // Purple
	Scalar(255, 128, 128)   // Pink
};

class KinectClass
{
public:
	HRESULT InitNUI();
	HRESULT UpdateFrame();
	void Stop();
	~KinectClass();

	Mat userMask;
	Mat colorImg;
	bool  haveBody;
	cv::Vec3f	modelLoc[20];
	int			ModelLeftJointIndex;
	int			ModelRightJointIndex;
	//vector<vector<cv::Point>> filterContours;
private:

	void DrawColor(HANDLE h); //get Kinect color data
	void DrawDepth(HANDLE h);  //get Kinect depth data
	void DrawSkeleton();		  //get Kinect Skeleton data

	INuiSensor * pNuiSensor;

	HANDLE      m_hNextVideoFrameEvent;  
	HANDLE      m_hNextDepthFrameEvent;  
	HANDLE		m_hNextSkeletonEvent;


	HANDLE      m_pVideoStreamHandle;  
	HANDLE      m_pDepthStreamHandle;  

	HANDLE      m_hEvNuiProcessStop;//用于结束的事件对象; 

	//---Image stream 分辨率-------------------------------
	static const NUI_IMAGE_RESOLUTION colorResolution = NUI_IMAGE_RESOLUTION_640x480;
	static const NUI_IMAGE_RESOLUTION depthResolution = NUI_IMAGE_RESOLUTION_640x480;




};


