
#include "cinder/app/AppBasic.h"
#include "cinder/ImageIo.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "cinder/Camera.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Vbo.h"
#include "cinder/ImageIo.h"
#include "cinder/params/Params.h"
#include "cinder/Utilities.h"

#include "Resources.h"
#include "VOpenNIHeaders.h"

using namespace ci;
using namespace ci::app;
using namespace std;


class ImageSourceKinectColor : public ImageSource 
{
public:
	ImageSourceKinectColor( uint8_t *buffer, int width, int height )
    : ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_RGB );
		setChannelOrder( ImageIo::RGB );
		setDataType( ImageIo::UINT8 );
	}
    
	~ImageSourceKinectColor() { }
    
	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );
        
		for( uint32_t row	 = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width * 3 );
	}
    
protected:
	uint32_t					_width, _height;
	uint8_t						*mData;
};


class ImageSourceKinectDepth : public ImageSource 
{
public:
	ImageSourceKinectDepth( uint16_t *buffer, int width, int height )
    : ImageSource(), mData( buffer ), _width(width), _height(height)
	{
		setSize( _width, _height );
		setColorModel( ImageIo::CM_GRAY );
		setChannelOrder( ImageIo::Y );
		setDataType( ImageIo::UINT16 );
	}
    
	~ImageSourceKinectDepth() { }
    
	virtual void load( ImageTargetRef target )
	{
		ImageSource::RowFunc func = setupRowFunc( target );
        
		for( uint32_t row = 0; row < _height; ++row )
			((*this).*func)( target, row, mData + row * _width );
	}
    
protected:
	uint32_t					_width, _height;
	uint16_t					*mData;
};


class KinectApp : public AppBasic, V::UserListener
{
public:
    
    // Set the mesh dimensions to match the Kinect's depth
	// image size
	static const int32_t MESH_WIDTH = 320;
	static const int32_t MESH_HEIGHT = 240;
    
	// Kinect
	float mBrightTolerance;
	float mDepth;
    
	static const int KINECT_COLOR_WIDTH = 640;	//1280;
	static const int KINECT_COLOR_HEIGHT = 480;	//1024;
	static const int KINECT_COLOR_FPS = 30;	//15;
	static const int KINECT_DEPTH_WIDTH = 640;
	static const int KINECT_DEPTH_HEIGHT = 480;
	static const int KINECT_DEPTH_FPS = 30;
    
    bool mRemoveBackground;
	bool mRemoveBackgroundPrev;
	ci::Vec3f mScale;
	
	ci::gl::Texture mTextureDepth;
	ci::gl::Texture::Format mTextureFormat;
    
	KinectApp();
	~KinectApp();
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void keyDown( KeyEvent event );
    void shutdown();
    
    
    // Camera
	ci::CameraPersp mCamera;
	ci::Vec3f mEyePoint;
	ci::Vec3f mLookAt;
	ci::Vec3f mRotation;
    
	// Shader
	void loadShaders();
	double mGlslVersion;
	bool mTransform;
	bool mTransformPrev;
	ci::gl::GlslProg mShader;
	ci::gl::GlslProg mShaderPassThru;
	ci::gl::GlslProg mShaderTransform;
    
	// Lighting
	ci::ColorAf mLightAmbient;
	ci::ColorAf mLightDiffuse;
	ci::Vec3f mLightPosition;
	float mLightShininess;
	ci::ColorAf mLightSpecular;
	
	// VBO
	void initMesh();
	std::vector<uint32_t> mVboIndices;
	ci::gl::VboMesh::Layout mVboLayout;
	std::vector<ci::Vec3f> mVboVertices;
	std::vector<ci::Vec2f> mVboTexCoords;
	ci::gl::VboMesh	mVboMesh;
    
	// Window
	ci::Colorf mBackgroundColor;
	float mElapsedFrames;
	float mElapsedSeconds;
	float mFrameRate;
	bool mFullScreen;
	bool mFullScreenPrev;
    
	// Debug
	ci::params::InterfaceGl mParams;
	//void screenShot();
	void trace(const std::string & message);
    
	void onNewUser( V::UserEvent event );
	void onLostUser( V::UserEvent event );
    
    float mMeshUvMix;
    uint16_t*				pixels;
    
	ImageSourceRef getColorImage()
	{
		// register a reference to the active buffer
		uint8_t *activeColor = _device0->getColorMap();
		return ImageSourceRef( new ImageSourceKinectColor( activeColor, KINECT_COLOR_WIDTH, KINECT_COLOR_HEIGHT ) );
	}
    
	ImageSourceRef getUserImage( int id )
	{
		_device0->getLabelMap( id, pixels );
		return ImageSourceRef( new ImageSourceKinectDepth( pixels, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}
    
	ImageSourceRef getDepthImage()
	{
		// register a reference to the active buffer
		uint16_t *activeDepth = _device0->getDepthMap();
		return ImageSourceRef( new ImageSourceKinectDepth( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	} 
    
	ImageSourceRef getDepthImage24()
	{
		// register a reference to the active buffer
		uint8_t *activeDepth = _device0->getDepthMap24();
		return ImageSourceRef( new ImageSourceKinectColor( activeDepth, KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT ) );
	}
	void prepareSettings( Settings *settings );
    
	V::OpenNIDeviceManager*	_manager;
	V::OpenNIDevice::Ref	_device0;
    
};



KinectApp::KinectApp() {
    
}

KinectApp::~KinectApp()
{
	if( pixels )
	{
		delete[] pixels;
		pixels = NULL;
	}
}

// Prepare window settings
void KinectApp::prepareSettings(ci::app::AppBasic::Settings * settings)
{
    
	// Set up window
	settings->setTitle("KinectApp");
	settings->setWindowSize(1024, 600);
	settings->setFrameRate(60.0f);
	settings->setFullScreen(false);
    
}

// This routine creates a vertex buffer object which 
// matches the dimensions of the Kinect depth image.
void KinectApp::initMesh()
{
    
	// Clear the VBO if it already exists
	if (mVboMesh)
		mVboMesh.reset();
    
	// Iterate through the mesh dimensions
	for (int32_t y = 0; y < MESH_HEIGHT; y++)
		for (int32_t x = 0; x < MESH_WIDTH; x++)
		{
            
			// Set the index of the vertex in the VBO so it is
			// numbered left to right, top to bottom
			mVboIndices.push_back(x + y * MESH_WIDTH);
            
			// Set the position of the vertex in world space
			mVboVertices.push_back(Vec3f((float)x - (float)MESH_WIDTH * 0.5f, (float)y - (float)MESH_HEIGHT * 0.5f, 0.0f));
            
			// Use percentage of the position for the texture coordinate
			mVboTexCoords.push_back(Vec2f((float)x / (float)MESH_WIDTH, (float)y / (float)MESH_HEIGHT));
            
		}
	
	// Create VBO
	mVboMesh = gl::VboMesh(mVboIndices.size(), mVboIndices.size(), mVboLayout, GL_POINTS);
	mVboMesh.bufferIndices(mVboIndices);
	mVboMesh.bufferPositions(mVboVertices);
	mVboMesh.bufferTexCoords2d(0, mVboTexCoords);
    
	// WORKAROUND: The bufferPositions call does not
	// unbind the VBO
	mVboMesh.unbindBuffers();
    
	// Clean up
	mVboIndices.clear();
	mVboTexCoords.clear();
	mVboVertices.clear();
    
	// Call the resize event to reset the camera 
	// and OpenGL state
	resize(ResizeEvent(getWindowSize()));
    
}


void KinectApp::setup()
{
    
    // Load the shader
	loadShaders();
    
    V::OpenNIDeviceManager::USE_THREAD = false;
	_manager = V::OpenNIDeviceManager::InstancePtr();
    
	_device0 = _manager->createDevice( V::NODE_TYPE_IMAGE | V::NODE_TYPE_DEPTH | V::NODE_TYPE_USER | V::NODE_TYPE_SCENE );	// Create manually.
	if( !_device0 ) 
	{
		DEBUG_MESSAGE( "(App)  Can't find a kinect device\n" );
        quit();
        shutdown();
	}
    _device0->addListener( this );
	_manager->start();
    
	pixels = new uint16_t[ KINECT_DEPTH_WIDTH*KINECT_DEPTH_HEIGHT ];
    
	
    // Set VBO layout
	mVboLayout.setStaticIndices();
	mVboLayout.setStaticPositions();
	mVboLayout.setStaticTexCoords2d();
    
	// Intialize camera
	mEyePoint = Vec3f::zero();
	mLookAt = Vec3f::zero();
    
	// Set up the light. This application does not actually use OpenGL 
	// lighting. Instead, it passes a light position and color 
	// values to the shader. Per fragment lighting is calculated in GLSL.
	mLightAmbient = ColorAf(0.5f, 0.5f, 0.5f, 1.0f);
	mLightDiffuse = ColorAf(0.25f, 0.25f, 0.25f, 1.0f);
	mLightPosition = Vec3f(0.0f, 250.0f, -100.0f);
	mLightShininess = 10.0f;
	mLightSpecular = ColorAf(0.75f, 0.75f, 0.75f, 1.0f);
    
	// Set default properties
	mBackgroundColor = Colorf(0.415f, 0.434f, 0.508f);
	mBrightTolerance = 0.05f;
	mDepth = 20.0f;
	mFrameRate = 0.0f;
	mFullScreen = isFullScreen();
	mFullScreenPrev = mFullScreen;
	mMeshUvMix = 0.2f;
	mRemoveBackground = true;
	mRemoveBackgroundPrev = mRemoveBackground;
	mTransform = true;
	mTransformPrev = mTransform;
	mScale = Vec3f(1.5f, 1.5f, 20.0f);
    
	// Create the parameters bar
	mParams = params::InterfaceGl("Parameters", Vec2i(250, 310));
	mParams.addSeparator("");
	mParams.addParam("Transform enabled", & mTransform, "key=a");
	mParams.addSeparator("");
	mParams.addParam("Bright tolerance", & mBrightTolerance, "min=0.000 max=1.000 step=0.001 keyDecr=b keyIncr=B");
	mParams.addParam("Depth", & mDepth, "min=0.0 max=2000.0 step=1.0 keyIncr=c keyDecr=C");
	mParams.addParam("Remove background", & mRemoveBackground, "key=d");
	mParams.addParam("Scale", & mScale);
	mParams.addSeparator("");
	mParams.addParam("Eye point", & mEyePoint);
	mParams.addParam("Look at", & mLookAt);
	mParams.addParam("Rotation", & mRotation);
	mParams.addSeparator("");
	mParams.addParam("Light position", & mLightPosition);
	mParams.addSeparator("");
	mParams.addParam("Frame rate", & mFrameRate, "", true);
	mParams.addParam("Full screen", & mFullScreen, "key=e");
	//mParams.addButton("Save screen shot", std::bind(& KinectApp::screenShot, this), "key=space");
	mParams.addButton("Quit", std::bind(& KinectApp::quit, this), "key=esc");
    
	// Initialize texture
	mTextureFormat.setInternalFormat(GL_RGBA_FLOAT32_ATI);
	mTextureDepth = gl::Texture(Surface32f(MESH_WIDTH, MESH_HEIGHT, false, SurfaceChannelOrder::RGBA), mTextureFormat);
    
	// Create VBO
	initMesh();


}

// This routine cleans up the application as it is exiting
void KinectApp::shutdown()
{
    
	// Stop Kinect input
	//mKinect.stop();
    
	// Clean up
	if (mTextureDepth)
		mTextureDepth.reset();
	mVboIndices.clear();
	if (mVboMesh)
		mVboMesh.reset();
	if (mShader)
		mShader.reset();
	if (mShaderPassThru)
		mShaderPassThru.reset();
	if (mShaderTransform)
		mShaderTransform.reset();
	mVboTexCoords.clear();
	mVboVertices.clear();
    
}


void KinectApp::mouseDown( MouseEvent event )
{
}

// This method borrows the term "trace" from ActionScript.
// This is a convenience method for writing a line to the
// console and (on Windows) debug window
void KinectApp::trace(const string & message)
{
    
	// Write to console and debug window
	console() << message << "\n";
#ifdef CINDER_MSW
	OutputDebugStringA((message + "\n").c_str());
#endif
    
}

void KinectApp::update()
{	
    
    
    // Update frame rate and elapsed time
	mElapsedFrames = (float)getElapsedFrames();
	mElapsedSeconds = (float)getElapsedSeconds();
	mFrameRate = getAverageFps();
    
    // update the OpenNI manager
    _manager->update();
    

    // Toggle fullscreen mode
	if (mFullScreen != mFullScreenPrev)
	{
		setFullScreen(mFullScreen);
		mFullScreenPrev = mFullScreen;
	}
    
	// Toggle between transform and pass-thru shaders
	if (mTransform != mTransformPrev)
	{
		mShader = mTransform ? mShaderTransform : mShaderPassThru;
		mTransformPrev = mTransform;
	}
    
    //V::UserBoneList boneList = _manager->getUser(1)->getBoneList();
    
    /*
    
	// Uses manager to handle users.
	if( _manager->hasUser(1) ) 
		mOneUserTex.update( getUserImage(1) );
    
    
    for( std::map<int, gl::Texture>::iterator it=mUsersTexMap.begin();
        it != mUsersTexMap.end();
        ++it )
    {
        it->second.update( getUserImage( it->first ) );
    }
    */
}


void KinectApp::draw()
{
	// Set up the scene
	gl::clear(mBackgroundColor, true);
	gl::setViewport(getWindowBounds());
	gl::setMatrices(mCamera);
    
	// Bind texture
	mTextureDepth.bind(0);
    
	// Bind shader
	mShader.bind();
	
	// Position world
	gl::pushModelView();
	gl::scale(-1.0f, -1.0f, -1.0f);
	gl::rotate(mRotation);
	
	// Set uniforms
	mShader.uniform("brightTolerance", mBrightTolerance);
	mShader.uniform("depth", mDepth);
	mShader.uniform("eyePoint", mEyePoint);
	mShader.uniform("height", (float)MESH_HEIGHT);
	mShader.uniform("lightAmbient", mLightAmbient);
	mShader.uniform("lightDiffuse", mLightDiffuse);
	mShader.uniform("lightPosition", mLightPosition);
	mShader.uniform("lightSpecular", mLightSpecular);
	mShader.uniform("mvp", gl::getProjection() * gl::getModelView());
	mShader.uniform("positions", 0);
	mShader.uniform("scale", mScale);
	mShader.uniform("shininess", mLightShininess);
	mShader.uniform("transform", mTransform);
	mShader.uniform("uvmix", mMeshUvMix);
	mShader.uniform("width", (float)MESH_WIDTH);
    
	// Draw VBO
	gl::draw(mVboMesh);
    
	// Stop drawing
	gl::popModelView();
	mShader.unbind();
	mTextureDepth.unbind();
    
	// Draw parameters
	params::InterfaceGl::draw();
}




void KinectApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_ESCAPE )
	{
		this->quit();
		this->shutdown();
	}
    
    
    int key = (int)event.getChar();    
    app::console() << "Key: " << key << std::endl;
    if( key >= 49 && key <= 57 )
    {
        app::console() << "Reset: " << (key-48) << std::endl;
        _device0->resetUser( key-48 ); // Abort calibration. the user still remains active.
    }
    //    app::console() << "Org User Count: " << _device0->getUserGenerator()->GetNumberOfUsers() << std::endl;
}

// Load GLSL shaders from resources
void KinectApp::loadShaders()
{
    
	try
	{
        
		// Find maximum number of output vertices for geometry shader.
		// We only need 6 to build a triangle strip quad. If the GPU
		// can't support 6 output vertices, it will draw as many as 
		// it can.
		int32_t maxGeomOutputVertices;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, & maxGeomOutputVertices);
		maxGeomOutputVertices = math<int32_t>::min(maxGeomOutputVertices, 6);
        
		// Get GLSL version
		string glslVersion = string((const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
		uint32_t spaceIndex = glslVersion.find_first_of(" ");
		if (spaceIndex > 0)
			glslVersion = glslVersion.substr(0, spaceIndex);
		trace("GLSL version: " + glslVersion);
		mGlslVersion = fromString<double>(glslVersion);
        
		// Use version 1.5 shaders, if possible
		if (mGlslVersion >= 1.5)
		{
            
			// We're loading the shader into two instances. The first 
			// passes through the geometry shader so you can see what 
			// the VBO looks like without the transformation. The 
			// second converts points to quads on the GPU. Note that we
			// set the number of output vertices to 1 for GL_POINTS.
			mShaderPassThru = gl::GlslProg(
                                           loadResource(RES_SHADER_VERT_150), 
                                           loadResource(RES_SHADER_FRAG_150), 
                                           loadResource(RES_SHADER_GEOM_150), 
                                           GL_POINTS, GL_POINTS, 1
                                           );
			mShaderTransform = gl::GlslProg(
                                            loadResource(RES_SHADER_VERT_150), 
                                            loadResource(RES_SHADER_FRAG_150), 
                                            loadResource(RES_SHADER_GEOM_150), 
                                            GL_POINTS, GL_TRIANGLE_STRIP, maxGeomOutputVertices
                                            );
            
		}
		else
		{
            
			// Same as above, but these are GLSL version 1.2 shaders.
			// These are a bit slower, but should work with older systems.
			mShaderPassThru = gl::GlslProg(
                                           loadResource(RES_SHADER_VERT_120), 
                                           loadResource(RES_SHADER_FRAG_120), 
                                           loadResource(RES_SHADER_GEOM_120), 
                                           GL_POINTS, GL_POINTS, 1
                                           );
			mShaderTransform = gl::GlslProg(
                                            loadResource(RES_SHADER_VERT_120), 
                                            loadResource(RES_SHADER_FRAG_120), 
                                            loadResource(RES_SHADER_GEOM_120), 
                                            GL_POINTS, GL_TRIANGLE_STRIP, maxGeomOutputVertices
                                            );
            
		}
        
	}
	catch (gl::GlslProgCompileExc & ex)
	{
        
		// Exit application if shaders failed to compile
		trace("Unable to compile shaders. Quitting.");
		trace(ex.what());
		quit();
        
	}
	catch (...)
	{
        
		// Exit application if shaders failed to load
		trace("Unable to load shaders. Quitting.");
		quit();
        
	}
    
	// Use transform shader by default
	mShader = mShaderTransform;
    
}


void KinectApp::onNewUser( V::UserEvent event )
{
	app::console() << "New User Added With ID: " << event.mId << std::endl;
    //mUsersTexMap.insert( std::make_pair( event.mId, gl::Texture(KINECT_DEPTH_WIDTH, KINECT_DEPTH_HEIGHT) ) );
}


void KinectApp::onLostUser( V::UserEvent event )
{
	app::console() << "User Lost With ID: " << event.mId << std::endl;
    //mUsersTexMap.erase( event.mId );
}

CINDER_APP_BASIC( KinectApp, RendererGl )
