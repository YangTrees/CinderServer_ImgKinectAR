#include "OneKinect.h"

//----------Kinect img Parameter-------------------------------------  
#define COLOR_WIDTH					640  
#define COLOR_HIGHT					480  
#define DEPTH_WIDTH					640  
#define DEPTH_HIGHT					480    
#define CHANNEL                     3  


HRESULT KinectClass::InitNUI()
{
	HRESULT hr;
	int n;
	hr = NuiGetSensorCount(&n);
	if (FAILED(hr))
	{
		return E_FAIL;
	}
	pNuiSensor = nullptr;

	if (SUCCEEDED(NuiCreateSensorByIndex(0, &pNuiSensor)))
	{
		pNuiSensor->NuiStatus();
	}

	hr	=	pNuiSensor->NuiInitialize(
		NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX|NUI_INITIALIZE_FLAG_USES_COLOR|NUI_INITIALIZE_FLAG_USES_SKELETON);  
	if(hr != S_OK )  
	{  
		cout<<"Nui Initialize Failed"<<endl;  
		return hr;  
	}


	//Open KINECT Color Camera 
	m_hNextVideoFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );  
	m_pVideoStreamHandle	= NULL;  
	hr	= pNuiSensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,colorResolution, 0, 2,
		m_hNextVideoFrameEvent, &m_pVideoStreamHandle);  
	if(FAILED( hr ) )  
	{  
		cout<<"Could not open image stream video"<<endl;  
		return hr;  
	}

	//Open KINECT Depth Camera 
	m_hNextDepthFrameEvent  = CreateEvent( NULL, TRUE, FALSE, NULL );  
	m_pDepthStreamHandle    = NULL;  
	hr = pNuiSensor->NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX,depthResolution, 0,
		2, m_hNextDepthFrameEvent,&m_pDepthStreamHandle);  
	if(FAILED( hr ) )  
	{  
		cout<<"Could not open depth stream video"<<endl;  
		return hr;  
	}

	//Open KINECT Skeleton 
	m_hNextSkeletonEvent	= CreateEvent( NULL, TRUE, FALSE, NULL );
	hr	=	NuiSkeletonTrackingEnable(m_hNextSkeletonEvent,0); 
	if(FAILED( hr ) )  
	{  
		cout<<"Could not open skeleton stream video"<<endl;  
		return hr;  
	}  
	m_hEvNuiProcessStop = CreateEvent(NULL,TRUE,FALSE,NULL); // To Finish the Kinect Camera Reading

	colorImg = Mat(COLOR_HIGHT,COLOR_WIDTH,CV_8UC4,Scalar(0,0,0));
	userMask = Mat(DEPTH_HIGHT,DEPTH_WIDTH,CV_8UC1,Scalar(0,0,0));
	haveBody = false;
	for(int i=0;i<NUI_SKELETON_POSITION_COUNT;i++)
	{
		modelLoc[i] = cv::Vec3f(0,0,0);
	}
	ModelLeftJointIndex = 7;
	ModelRightJointIndex = 11;
	return hr;  
}


HRESULT KinectClass::UpdateFrame()
{
	//Update Kinect Frame Data
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hEvNuiProcessStop, 0))  
	{  
		CloseHandle(m_hEvNuiProcessStop);  
		m_hEvNuiProcessStop= NULL;
		CloseHandle(m_hNextSkeletonEvent);  
		CloseHandle(m_hNextDepthFrameEvent); 
		CloseHandle(m_hNextVideoFrameEvent);
		return S_FALSE;  
	}
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hNextVideoFrameEvent, 0))  
	{  
		DrawColor(m_pVideoStreamHandle);
	}
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hNextDepthFrameEvent, 0))  
	{  
		DrawDepth(m_pDepthStreamHandle);  
	}
	if(WAIT_OBJECT_0 == WaitForSingleObject(m_hNextSkeletonEvent, 0))  
	{  
		DrawSkeleton();  
	}
	return S_OK;
}

void KinectClass::DrawColor(HANDLE h)  
{  
	//Read Color Frame
	NUI_IMAGE_FRAME pImageFrame;  
	HRESULT hr = pNuiSensor->NuiImageStreamGetNextFrame( h, 0, &pImageFrame ); 
	if(FAILED( hr ) )  
	{  
		cout<<"GetColor Image Frame Failed"<<endl;  
		return ;  
	}  
	INuiFrameTexture* pTexture = pImageFrame.pFrameTexture;  
	NUI_LOCKED_RECT LockedRect;  
	pTexture->LockRect(0, &LockedRect, NULL, 0 );  
	if(LockedRect.Pitch != 0 )  
	{  
		BYTE* pBuffer = (BYTE*) LockedRect.pBits;  
		if(true)
		{
			Mat temp(COLOR_HIGHT,COLOR_WIDTH,CV_8UC4,pBuffer);
			temp.copyTo(colorImg);
		}
	}  
	pNuiSensor->NuiImageStreamReleaseFrame(h, &pImageFrame );  
	return ;  
}

void KinectClass::DrawDepth(HANDLE h)  
{  
	//Read Depth Frame
	NUI_IMAGE_FRAME pImageFrame ;  
	HRESULT hr = pNuiSensor->NuiImageStreamGetNextFrame( h, 0, &pImageFrame );  
	if(FAILED( hr ) )  
	{  
		cout<<"GetDepth Image Frame Failed"<<endl;  
		return;  
	}
	INuiFrameTexture* pTexture = pImageFrame.pFrameTexture;  

	NUI_LOCKED_RECT LockedRect;  
	pTexture->LockRect(0, &LockedRect, NULL, 0 );
	if(LockedRect.Pitch != 0 )  
	{  
		BYTE* pBuff = (BYTE*) LockedRect.pBits;
		//OpenCV显示深度视频
		if(true)
		{
			Mat depthTmp(DEPTH_HIGHT,DEPTH_WIDTH,CV_16U,pBuff);
			//imshow("Depth",depthTmp);

			userMask.setTo(0);

			//显示骨骼人体区域信息;
			for (int y=0; y<DEPTH_HIGHT; y++)
			{
				const USHORT* p_depthTmp = depthTmp.ptr<USHORT>(y); 
				uchar* pUserMask = userMask.ptr<uchar>(y); 
				for (int x=0; x<DEPTH_WIDTH; x++)
				{
					USHORT depthValue = p_depthTmp[x];
					if (depthValue != 63355)
					{
						USHORT playerIndex	= NuiDepthPixelToPlayerIndex(depthValue);
						if(playerIndex>0&&playerIndex<7)
						{
								pUserMask[x] = 255;
						}
						else
						{
								pUserMask[x] = 0;
						}
					}
					else
					{
						pUserMask[x] = 0;
					}

				}
			}
			
		}
		
		
	}  
	pNuiSensor->NuiImageStreamReleaseFrame(h, &pImageFrame ); 
	return ;  
}

void KinectClass::DrawSkeleton()  
{  
	NUI_SKELETON_FRAME SkeletonFrame;  
	cv::Point pt[20];    
	HRESULT hr = NuiSkeletonGetNextFrame( 0, &SkeletonFrame );  
	if(FAILED( hr ) )  
	{  
		cout<<"GetSkeleton Image Frame Failed"<<endl;  
		return ;  
	}

	bool bFoundSkeleton = false;  
	for(int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )  
	{  
		if(SkeletonFrame.SkeletonData[i].eTrackingState == NUI_SKELETON_TRACKED )  
		{  
			bFoundSkeleton = true;  
		}  
	}
	haveBody = bFoundSkeleton;
	float minZ = 10;
	
	//Has skeletons!  
	if(bFoundSkeleton )  
	{  
		NuiTransformSmooth(&SkeletonFrame,NULL);  
		//cout<<"skeleton num:"<<NUI_SKELETON_COUNT<<endl;
		for(int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )  
		{  
			NUI_SKELETON_TRACKING_STATE trackingState = SkeletonFrame.SkeletonData[i].eTrackingState;
			if(trackingState == NUI_SKELETON_TRACKED )  //骨骼位置跟踪成功，则直接定位;
			{  
				//NUI_SKELETON_DATA *pSkel = &(SkeletonFrame.SkeletonData[i]);
				NUI_SKELETON_DATA  SkelData = SkeletonFrame.SkeletonData[i];
				Point jointPositions[NUI_SKELETON_POSITION_COUNT];

				for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j)
				{
					LONG x, y;
					USHORT depth;
					//cout<<j<<" :("<<SkelData.SkeletonPositions[j].x<<","<<SkelData.SkeletonPositions[j].y<<") ";
					NuiTransformSkeletonToDepthImage(SkelData.SkeletonPositions[j], &x, &y, &depth, depthResolution);
					//circle(depthRGB, Point(x,y), 5, Scalar(255,255,255), -1, CV_AA);
					jointPositions[j] = Point(x, y);
				}

				for (int j = 0; j < NUI_SKELETON_POSITION_COUNT; ++j)
				{
					if (SkelData.eSkeletonPositionTrackingState[j] == NUI_SKELETON_POSITION_TRACKED)
					{
						//circle(colorImg,jointPositions[j],5,SKELETON_COLORS[i],-1,CV_AA);
						//circle(colorImg,jointPositions[j],6,Scalar(0,0,0),1,CV_AA);
					}
					else if (SkelData.eSkeletonPositionTrackingState[j] == NUI_SKELETON_POSITION_INFERRED)
					{
						//circle(colorImg, jointPositions[j], 5, Scalar(255,255,255), -1, CV_AA);
					}
				}
				if(SkelData.SkeletonPositions[0].z<minZ)
				{
					minZ = SkelData.SkeletonPositions[0].z; 
					for(int k=0; k<NUI_SKELETON_POSITION_COUNT; k++)
					{
						modelLoc[k] = Vec3f(SkelData.SkeletonPositions[k].x,
							SkelData.SkeletonPositions[k].y,
							SkelData.SkeletonPositions[k].z);
					}
				}
				
				//cout<<endl;
			}
			else if(trackingState == NUI_SKELETON_POSITION_INFERRED) //如果骨骼位置跟踪未成功，通过推测定位骨骼位置;
			{
				LONG x, y;
				USHORT depth=0;
				NuiTransformSkeletonToDepthImage(SkeletonFrame.SkeletonData[i].Position,&x, &y,&depth,depthResolution);
				//cout<<SkeletonFrame.SkeletonData[i].Position.x<<";"<<SkeletonFrame.SkeletonData[i].Position.y<<endl;
				circle(colorImg, Point(x, y), 7, CV_RGB(0,0,0), CV_FILLED);
			}

		}

	}  
	
	return ;  
}

void KinectClass::Stop()
{
	SetEvent(m_hEvNuiProcessStop);
}

KinectClass::~KinectClass()
{
	NuiShutdown();
}

