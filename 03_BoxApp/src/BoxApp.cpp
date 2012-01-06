// Includes
#include <cinder/app/AppBasic.h>
#include <cinder/Arcball.h>
#include <cinder/Camera.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Vbo.h>
#include <cinder/ImageIo.h>
#include <cinder/params/Params.h>
#include <cinder/Utilities.h>
#include <Resources.h>

/*
 * This application demonstrates how to create a vertex
 * buffer object made of points, then convert each point to
 * a box with rotation, normals, texture mapping coordinates, 
 * and lighting applied. We'll also step up to GLSL 1.5 while 
 * making the application reverse compatible down to GLSL 1.2.
 */

// GPU box mesh
class BoxApp : public ci::app::AppBasic 
{

public:

	// Cinder callbacks
	void draw();
	void mouseDown(ci::app::MouseEvent event);
	void mouseDrag(ci::app::MouseEvent event);
	void mouseMove(ci::app::MouseEvent event);
	void mouseWheel(ci::app::MouseEvent event);
	void prepareSettings(ci::app::AppBasic::Settings * settings);
	void resize(ci::app::ResizeEvent event);
	void setup();
	void shutdown();
	void update();

private:

	// Cursor
	ci::Vec2i mCursor;

	// Camera
	ci::Arcball mArcball;
	ci::CameraPersp mCamera;
	ci::Vec3f mEyePoint;
	ci::Vec3f mLookAt;

	// Mesh parameters
	float mMeshAlpha;
	int32_t mMeshHeight;
	int32_t mMeshHeightPrev;
	ci::Vec3f mMeshOffset;
	ci::Vec3f mMeshRotation;
	float mMeshScale;
	float mMeshUvMix;
	float mMeshWaveAmplitude;
	float mMeshWaveSpeed;
	float mMeshWaveWidth;
	int32_t mMeshWidth;
	int32_t mMeshWidthPrev;

	// Box
	ci::Vec3f mBoxDimensions;
	bool mBoxEnabled;
	bool mBoxEnabledPrev;
	float mBoxRotationSpeed;

	// Shader
	void loadShader();
	double mGlslVersion;
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
	void screenShot();
	void trace(const std::string & message);

};

// Import namespaces
using namespace ci;
using namespace ci::app;
using namespace std;

// Renders the scene
void BoxApp::draw()
{

	// Set up scene
	gl::clear(mBackgroundColor, true);
	gl::setViewport(getWindowBounds());
	gl::setMatrices(mCamera);
	
	// Move into position
	gl::pushModelView();
	gl::translate(mMeshOffset);
	gl::rotate(mMeshRotation);

	// Bind and configure shader
	mShader.bind();
	mShader.uniform("alpha", mMeshAlpha);
	mShader.uniform("amp", mMeshWaveAmplitude);
	mShader.uniform("eyePoint", mEyePoint);
	mShader.uniform("lightAmbient", mLightAmbient);
	mShader.uniform("lightDiffuse", mLightDiffuse);
	mShader.uniform("lightPosition", mLightPosition);
	mShader.uniform("lightSpecular", mLightSpecular);
	mShader.uniform("mvp", gl::getProjection() * gl::getModelView());
	mShader.uniform("phase", mElapsedSeconds);
	mShader.uniform("rotation", mBoxRotationSpeed);
	mShader.uniform("scale", mMeshScale);
	mShader.uniform("dimensions", Vec4f(mBoxDimensions));
	mShader.uniform("shininess", mLightShininess);
	mShader.uniform("speed", mMeshWaveSpeed);
	mShader.uniform("transform", mBoxEnabled);
	mShader.uniform("uvmix", mMeshUvMix);
	mShader.uniform("width", mMeshWaveWidth);
	
	// Draw VBO
	gl::draw(mVboMesh);

	// Stop drawing
	mShader.unbind();
	gl::popModelView();

	// Draw the params interface
	params::InterfaceGl::draw();

}

// This routine creates a vertex buffer object. This is basically
// just a grid of evenly spaced vertices (points).
void BoxApp::initMesh()
{

	// Iterate through the grid dimensions
	for (int32_t y = 0; y < mMeshHeight; y++)
		for (int32_t x = 0; x < mMeshWidth; x++)
		{

			// Set the index of the vertex in the VBO so it is
			// numbered left to right, top to bottom
			mVboIndices.push_back(x + y * mMeshWidth);

			// Set the position of the vertex in world space
			mVboVertices.push_back(Vec3f((float)x - (float)mMeshWidth * 0.5f, (float)y - (float)mMeshHeight * 0.5f, 0.0f));

		}

	// Clear the VBO if it already exists
	if (mVboMesh)
		mVboMesh.reset();
	
	// Create VBO
	mVboMesh = gl::VboMesh(mVboVertices.size(), mVboIndices.size(), mVboLayout, GL_POINTS);
	mVboMesh.bufferIndices(mVboIndices);
	mVboMesh.bufferPositions(mVboVertices);

	// WORKAROUND: The bufferPositions call does not
	// unbind the VBO
	mVboMesh.unbindBuffers();

	// Clean up
	mVboIndices.clear();
	mVboVertices.clear();

	// Resize the window to reset view
	resize(ResizeEvent(getWindowSize()));

}

// Load GLSL shaders from resources
void BoxApp::loadShader()
{

	try
	{

		// Find maximum number of output vertices for geometry shader.
		// We only need 32 to build a triangle strip box. If the GPU
		// can't support 32 output vertices, it will draw as many as 
		// it can.
		int32_t maxGeomOutputVertices;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, & maxGeomOutputVertices);
		maxGeomOutputVertices = math<int32_t>::min(maxGeomOutputVertices, 32);

		// Get GLSL version
		string glslVersion = string((const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
		uint32_t spaceIndex = glslVersion.find_first_of(" ");
		if (spaceIndex > 0)
			glslVersion = glslVersion.substr(0, spaceIndex);
		trace("GLSL version: " + glslVersion);
		mGlslVersion = fromString<double>(glslVersion);

		// Use version 1.5 shaders if possible
		if (mGlslVersion >= 1.5)
		{

			// We're loading the shaders into two instances. The first 
			// passes through the geometry shader so you can see what 
			// the VBO looks like without the transformation. The 
			// second converts points to boxes on the GPU. Note that we
			// set the number of output vertices to 1 for GL_POINTS.
			mShaderPassThru = gl::GlslProg(
				loadResource(RES_SHADER_VERT_150), 
				loadResource(RES_SHADER_FRAG_150), 
				loadResource(RES_SHADER_GEOM_150), 
				GL_POINTS, GL_POINTS, 1);
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

// Handles mouse down event
void BoxApp::mouseDown(MouseEvent event)
{

	// Set cursor position
	mCursor = event.getPos();

	// Update arcball, set camera rotation if holding ALT
	mArcball.mouseDown(event.getPos());
	if (event.isAltDown())
		mMeshRotation = mArcball.getQuat().v;

}

// Handles mouse drag event
void BoxApp::mouseDrag(MouseEvent event)
{

	// Get velocity
	Vec2i velocity = mCursor - event.getPos();

	// Set cursor position
	mCursor = event.getPos();

	// Update arcball, set camera rotation if holding ALT,
	// or drag the model if holding SHIFT
	mArcball.mouseDrag(event.getPos());
	if (event.isAltDown())
		mMeshRotation = mArcball.getQuat().v * math<float>::abs(mEyePoint.z);
	else if (event.isShiftDown())
		mMeshOffset += Vec3f((float)velocity.x, (float)velocity.y, 0.0f);

}

// Handles mouse move event
void BoxApp::mouseMove(MouseEvent event)
{

	// Set cursor position
	mCursor = event.getPos();

}

// Handles mouse wheel
void BoxApp::mouseWheel(MouseEvent event)
{

	// Move the mesh along the Z axis using the
	// mouse wheel
	mMeshOffset.z -= event.getWheelIncrement();

}

// Prepare window settings
void BoxApp::prepareSettings(ci::app::AppBasic::Settings * settings)
{

	// Set up window
	settings->setTitle("BoxApp");
	settings->setWindowSize(1024, 600);
	settings->setFrameRate(60.0f);
	settings->setFullScreen(false);

}

// Handles window resize event
void BoxApp::resize(ResizeEvent event)
{

	// Reset camera
	mEyePoint = Vec3f(0.0f, 0.0f, -500.0f);
	mLookAt = Vec3f::zero();
	mCamera.lookAt(mEyePoint, mLookAt);
	mCamera.setPerspective(60.0f, getWindowAspectRatio(), 0.1f, 15000.0f);
	gl::setMatrices(mCamera);

	// Reset arcball
	mArcball.setWindowSize(getWindowSize());
	mArcball.setCenter(getWindowCenter());
	mArcball.setRadius((float)getWindowSize().y * 0.5f);

	// Set up OpenGL
	glEnable(GL_DEPTH_TEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POLYGON_SMOOTH);
	gl::enableAlphaBlending();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPointSize(3.0f);
	gl::color(ColorAf::white());

}

// Take screen shot
void BoxApp::screenShot()
{

	// Save PNG of screen to application directory
	writeImage(fs::path(getAppPath().generic_string() + "frame" + toString(getElapsedFrames()) + ".png"), copyWindowSurface());

}

// Set up
void BoxApp::setup()
{

	// Load the shader
	loadShader();

	// Set VBO layout
	mVboLayout.setStaticIndices();
	mVboLayout.setStaticPositions();

	// Intialize camera
	mEyePoint = Vec3f::zero();
	mLookAt = Vec3f::zero();

	// Initialize cursor position
	mCursor = Vec2i::zero();

	// Set background color
	mBackgroundColor = Colorf(0.415f, 0.434f, 0.508f);
	
	// Set fullscreen flag for parameters
	mFullScreen = isFullScreen();
	mFullScreenPrev = mFullScreen;
	
	// Set default box dimensions
	mBoxDimensions = Vec3f(3.0f, 6.0f, 1.0f);
	mBoxEnabled = true;
	mBoxEnabledPrev = mBoxEnabled;
	mBoxRotationSpeed = 0.01f;

	// Set default mesh dimensions
	mMeshAlpha = 0.8f;
	mMeshHeight = 28;
	mMeshHeightPrev = mMeshHeight;
	mMeshOffset = Vec3f(-120.0f, -180.0f, 0.0f);
	mMeshRotation = Vec3f(75.0f, 0.0f, 330.0f);
	mMeshScale = 20.0f;
	mMeshUvMix = 0.133f;
	mMeshWaveAmplitude = 4.0f;
	mMeshWaveSpeed = 0.667f;
	mMeshWaveWidth = 0.105f;
	mMeshWidth = 112;
	mMeshWidthPrev = mMeshWidth;

	// Set up the light. This application does not actually use OpenGL 
	// lighting. Instead, it passes a light position and color 
	// values to the shader. Per-fragment lighting is calculated in GLSL.
	mLightAmbient = ColorAf(0.5f, 0.5f, 0.5f, 1.0f);
	mLightDiffuse = ColorAf(0.25f, 0.25f, 0.25f, 1.0f);
	mLightPosition = Vec3f(300.0f, 250.0f, -400.0f);
	mLightShininess = 20.0f;
	mLightSpecular = ColorAf(0.75f, 0.75f, 0.75f, 1.0f);

	// Create the parameters bar
	mParams = params::InterfaceGl("Parameters", Vec2i(250, 400));
	mParams.addSeparator("");
	mParams.addText("Hold ALT to rotate");
	mParams.addText("Hold SHIFT to drag");
	mParams.addText("Wheel to zoom");
	mParams.addSeparator("");
	mParams.addParam("Box transform enabled", & mBoxEnabled, "key=a");
	mParams.addParam("Box rotation speed", & mBoxRotationSpeed, "min=0.00000 max=1.00000 step=0.00001 keyDecr=b keyIncr=B");
	mParams.addParam("Box dimensions", & mBoxDimensions);
	mParams.addSeparator("");
	mParams.addParam("Mesh height", & mMeshHeight, "min=1 max=2048 step=1 keyDecr=c keyIncr=C");
	mParams.addParam("Mesh width", & mMeshWidth, "min=1 max=2048 step=1 keyDecr=d keyIncr=D");
	mParams.addParam("Mesh scale", & mMeshScale, "min=0.1 max=30000.0 step=0.1 keyDecr=e keyIncr=E");
	mParams.addParam("Mesh wave amplitude", & mMeshWaveAmplitude, "min=0.000 max=30000.000 step=0.001 keyDecr=f keyIncr=F");
	mParams.addParam("Mesh wave speed", & mMeshWaveSpeed, "min=0.000 max=100.000 step=0.001 keyDecr=g keyIncr=G");
	mParams.addParam("Mesh wave width", & mMeshWaveWidth, "min=0.000 max=30000.000 step=0.001 keyDecr=h keyIncr=H");
	mParams.addSeparator("");
	mParams.addParam("Light position", & mLightPosition);
	mParams.addSeparator("");
	mParams.addParam("Frame rate", & mFrameRate, "", true);
	mParams.addParam("Full screen", & mFullScreen, "key=i");
	mParams.addButton("Save screen shot", std::bind(& BoxApp::screenShot, this), "key=space");
	mParams.addButton("Quit", std::bind(& BoxApp::quit, this), "key=esc");
	
	// Create mesh
	initMesh();

}

// This routine cleans up the application as it is exiting
void BoxApp::shutdown()
{

	// Clean up
	if (mShader)
		mShader.reset();
	if (mShaderPassThru)
		mShaderPassThru.reset();
	if (mShaderTransform)
		mShaderTransform.reset();
	mVboIndices.clear();
	mVboVertices.clear();
	if (mVboMesh)
		mVboMesh.reset();

}

// This method borrows the term "trace" from ActionScript.
// This is a convenience method for writing a line to the
// console and (on Windows) debug window
void BoxApp::trace(const string & message)
{

	// Write to console and debug window
	console() << message << "\n";
#ifdef CINDER_MSW
	OutputDebugStringA((message + "\n").c_str());
#endif

}

// Runs update logic
void BoxApp::update()
{

	// Update frame rate and elapsed time
	mElapsedFrames = (float)getElapsedFrames();
	mElapsedSeconds = (float)getElapsedSeconds();
	mFrameRate = getAverageFps();

	// Update mesh if dimensions change
	if (mMeshHeight != mMeshHeightPrev || 
		mMeshWidth != mMeshWidthPrev)
	{
		initMesh();
		mMeshHeightPrev = mMeshHeight;
		mMeshWidthPrev = mMeshWidth;
	}

	// Toggle fullscreen mode
	if (mFullScreen != mFullScreenPrev)
	{
		setFullScreen(mFullScreen);
		mFullScreenPrev = mFullScreen;
	}

	// Toggle between transform and pass-thru shaders
	if (mBoxEnabled != mBoxEnabledPrev)
	{
		mShader = mBoxEnabled ? mShaderTransform : mShaderPassThru;
		mBoxEnabledPrev = mBoxEnabled;
	}

	// Update camera
	mCamera.lookAt(mEyePoint, mLookAt);

}

// Run application
CINDER_APP_BASIC(BoxApp, RendererGl)
