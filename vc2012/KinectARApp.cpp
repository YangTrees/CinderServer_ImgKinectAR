#include "cinder/app/AppBasic.h"
#include "cinder/Surface.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/Camera.h"
#include "cinder/Text.h"
#include "cinder/Arcball.h"
#include "cinder/ObjLoader.h"
#include "cinder/TriMesh.h"
#include "cinder/gl/Vbo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/params/Params.h"

#include <string>

#include <Shlobj.h>
#include "windows.h"
#include "OneKinect.h"

using namespace std;
using namespace ci;
using namespace ci::app;

class KinectARApp : public AppBasic {
  public:
	~KinectARApp();
	void setup();
	void resize();
	void update();
	void draw();
	
	
private:

	gl::Texture			mTexture;
	//Kinect Color Texture
	gl::Texture			mKinectTex;

	// transformations (translate, rotate, scale) of the model
	Matrix44f			mTransform;

	// the model
	TriMesh				mMesh;
	// our camera
	MayaCamUI	mMayaCam;

	KinectClass			mKinectHelper;

	params::InterfaceGlRef	mParams;
	float				angle;
	float				offsetZ;
	float				mScale;
	float				mPercentLoc;
};

void KinectARApp::setup()
{
	mKinectHelper.InitNUI();

	angle = 62.0f;
	offsetZ = 1.28f;
	mScale = 1.0f;
	mPercentLoc = 0.5f;
	mParams = params::InterfaceGl::create( "UI Console", ci::Vec2i( 225, 200 ) );
	mParams->addParam( "Camera FOV", &angle );
	mParams->addParam( "Scale", &mScale ).max(10.0f).min(0.1f).step(0.1);
	mParams->addParam( "Y_Adjust ", &mPercentLoc ).max(1.0f).min(0.1f).step(0.01);
	mParams->addSeparator();

	
	string modelTexFilename = "ModelTexture.jpg";
	mTexture = loadImage(loadAsset(modelTexFilename));

	ObjLoader loader( (DataSourceRef)loadAsset("Model.obj"));
	loader.load( &mMesh );


	CameraPersp			initialCam;
	initialCam.lookAt(ci::Vec3f::zero(),ci::Vec3f( 0, 0, 10 ),ci::Vec3f( 0, 1, 0 ));
	initialCam.setPerspective( angle, getWindowAspectRatio(), 0.001, 1000 );
	mMayaCam.setCurrentCam( initialCam );

	
	glEnable( GL_TEXTURE_2D );
	gl::enableDepthRead();
	gl::enableDepthWrite();	
}

void KinectARApp::resize()
{
	// now tell our Camera that the window aspect ratio has changed
	CameraPersp cam = mMayaCam.getCamera();
	cam.setAspectRatio( getWindowAspectRatio() );
	// and in turn, let OpenGL know we have a new camera
	mMayaCam.setCurrentCam( cam );
	

}

void KinectARApp::update()
{
	//set the OpenGL Camera
	CameraPersp cam = mMayaCam.getCamera();
	cam.setFov(angle);
	mMayaCam.setCurrentCam( cam );
	//update Kinect data
	mKinectHelper.UpdateFrame();
	
	int jointIndex = 2;
	if (mKinectHelper.haveBody)
	{
		ci::Vec3f trans3f(-(mKinectHelper.modelLoc[1][0]*mPercentLoc+mKinectHelper.modelLoc[2][0]*(1-mPercentLoc))*offsetZ,
						   (mKinectHelper.modelLoc[1][1]*mPercentLoc+mKinectHelper.modelLoc[2][1]*(1-mPercentLoc))*offsetZ,
						   (mKinectHelper.modelLoc[1][2]*mPercentLoc+mKinectHelper.modelLoc[2][2]*(1-mPercentLoc)));
		mTransform.setToIdentity();
		mTransform.translate(trans3f);
		mTransform.rotate( ci::Vec3f::xAxis(), sinf( (float) getElapsedSeconds() * 3.0f ) * 0.08f );
		mTransform.rotate( ci::Vec3f::yAxis(), (float) getElapsedSeconds() * 0.1f );
		mTransform.rotate( ci::Vec3f::zAxis(), sinf( (float) getElapsedSeconds() * 4.3f ) * 0.09f );
		mTransform.scale(ci::Vec3f(0.001f,0.001f,0.001f)*mScale);

	}
	
}

void KinectARApp::draw()
{
	gl::setMatrices( mMayaCam.getCamera() );
	//gl::clear(Color::black());
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);   
	//draw the Kinect Texture
	mKinectTex = gl::Texture(mKinectHelper.colorImg.data,GL_BGRA,mKinectHelper.colorImg.cols,mKinectHelper.colorImg.rows);
	mKinectTex.bind();
	float param = 1.0f;
	float coodX = 4.0f*param;
	float coodY = 3.0f*param;
	float coodZ = 5.0f*param;
	glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(coodX, coodY, coodZ);   
        glTexCoord2f(1.0f, 0.0f); glVertex3f(-coodX, coodY, coodZ);   
        glTexCoord2f(1.0f, 1.0f); glVertex3f(-coodX, -coodY,  coodZ);   
        glTexCoord2f(0.0f, 1.0f); glVertex3f(coodX, -coodY,  coodZ);   
    glEnd(); 
	
	//draw the mesh
	if (mKinectHelper.haveBody)
	{
		mTexture.bind();
		gl::color( Color::white() );
		gl::pushModelView();
		gl::multModelView( mTransform );
		gl::draw( mMesh);
		gl::popModelView();

	}
	//draw the UI
	mParams->draw();
}
KinectARApp::~KinectARApp()
{
	mKinectHelper.Stop();
}


CINDER_APP_BASIC( KinectARApp, RendererGl )
