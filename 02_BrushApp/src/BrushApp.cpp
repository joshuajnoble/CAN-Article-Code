// Includes
#include <cinder/app/AppBasic.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>
#include <cinder/ImageIo.h>
#include <cinder/params/Params.h>
#include <cinder/Rand.h>
#include <cinder/Utilities.h>
#include <Resources.h>

/*
 * This application demonstrates the basics of how
 * to load and configure a geometry shader. It converts 
 * points to shapes generated in GLSL.
 */

// GPU box mesh
class BrushApp : public ci::app::AppBasic 
{

public:

	// Cinder callbacks
	void draw();
	void mouseDrag(ci::app::MouseEvent event);
	void prepareSettings(ci::app::AppBasic::Settings * settings);
	void resize(ci::app::ResizeEvent event);
	void setup();
	void shutdown();
	void update();

private:

	// Brush
	float mSize;
	ci::gl::Texture mTexture;
	bool mTransform;
	bool mTransformPrev;

	// Point list
	void clearPoints();
	std::vector<ci::Vec2f> mPoints;

	// Shader
	void loadShader();
	double mGlslVersion;
	ci::gl::GlslProg mShader;
	ci::gl::GlslProg mShaderPassThru;
	ci::gl::GlslProg mShaderTransform;
	
	// Window
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
void BrushApp::draw()
{

	// Set up scene
	gl::clear(ColorAf::black(), true);
	gl::setViewport(getWindowBounds());
	gl::setMatricesWindow(getWindowSize());

	// Bind brush texture
	mTexture.bind(0);

	// Bind and configure shader
	mShader.bind();
	mShader.uniform("aspect", getWindowAspectRatio());
	mShader.uniform("size", mSize);
	mShader.uniform("tex", 0);
	mShader.uniform("transform", mTransform);

	// Draw points
	glBegin(GL_POINTS);
	for (vector<Vec2f>::const_iterator pointIt = mPoints.cbegin(); pointIt != mPoints.cend(); ++pointIt)
		gl::vertex(* pointIt);
	glEnd();

	// Unbind shader drawing
	mShader.unbind();

	// Unbind brush texture
	mTexture.unbind();

	// Draw the params interface
	params::InterfaceGl::draw();

}

// Clear point list
void BrushApp::clearPoints()
{
	mPoints.clear();
}

// Load GLSL shaders from resources
void BrushApp::loadShader()
{

	try
	{

		// Find maximum number of output vertices for geometry shader
		int32_t maxGeomOutputVertices;
		glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, & maxGeomOutputVertices);

		// Get GLSL version
		string glslVersion = string((const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
		uint32_t spaceIndex = glslVersion.find_first_of(" ");
		if (spaceIndex > 0)
			glslVersion = glslVersion.substr(0, spaceIndex);
		trace("GLSL version: " + glslVersion);
		mGlslVersion = fromString<double>(glslVersion);

		// GLSL 1.2 or better is needed to run this application
		if (mGlslVersion >= 1.2)
		{

			// We're loading the shaders into two instances. The first 
			// passes through the geometry shader so you can see what 
			// the points look like without the transformation. The 
			// second converts points to shapes on the GPU. Note that we
			// set the number of output vertices to 1 for GL_POINTS.
			mShaderPassThru = gl::GlslProg(
				loadResource(RES_SHADER_VERT), 
				loadResource(RES_SHADER_FRAG), 
				loadResource(RES_SHADER_GEOM), 
				GL_POINTS, GL_POINTS, 1
				);
			mShaderTransform = gl::GlslProg(
				loadResource(RES_SHADER_VERT), 
				loadResource(RES_SHADER_FRAG), 
				loadResource(RES_SHADER_GEOM), 
				GL_POINTS, GL_TRIANGLE_STRIP, maxGeomOutputVertices
				);

		}
		else
		{

			// Exit application if shaders aren't supported
			trace("GLSL 1.2 or better required. Quitting.");
			quit();

		}

	}
	catch (gl::GlslProgCompileExc ex)
	{

		// Exit application if shaders failed to compile
		trace("Unable to compile shaders. Quitting.");
		trace(ex.what());
		quit();

	}

	// Use transform shader by default
	mShader = mShaderTransform;

}

// Handles mouse drag
void BrushApp::mouseDrag(ci::app::MouseEvent event)
{

	// Add a brush
	mPoints.push_back(Vec2f((float)event.getPos().x, (float)event.getPos().y));

}

// Prepare window settings
void BrushApp::prepareSettings(ci::app::AppBasic::Settings * settings)
{

	// Set up window
	settings->setTitle("BrushApp");
	settings->setWindowSize(1024, 600);
	settings->setFrameRate(60.0f);
	settings->setFullScreen(false);

}

// Handles window resize event
void BrushApp::resize(ResizeEvent event)
{

	// Set up OpenGL
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POLYGON_SMOOTH);
	gl::enableAlphaBlending();
	gl::enableAdditiveBlending();
    glAlphaFunc(GL_ZERO, GL_SRC_COLOR);
	glPointSize(3.0f);
	gl::color(ColorAf::white());

}

// Take screen shot
void BrushApp::screenShot()
{

	// Save PNG of screen to application directory
	writeImage(fs::path(getAppPath().generic_string() + "frame" + toString(getElapsedFrames()) + ".png"), copyWindowSurface());

}

// Set up
void BrushApp::setup()
{

	// Load the shader
	loadShader();

	// Load brush
	mTexture = gl::Texture(loadImage(loadResource(RES_IMAGE_BRUSH)));

	// Set default parameters
	mFullScreen = isFullScreen();
	mFullScreenPrev = mFullScreen;
	mSize = 0.1f;
	mTransform = true;

	// Create the parameters bar
	mParams = params::InterfaceGl("Parameters", Vec2i(210, 200));
	mParams.addSeparator("");
	mParams.addParam("Brush size", & mSize, "min=0.0000 max=1.0000 step=0.0001 keyDecr=a keyIncr=A");
	mParams.addParam("Brush transform", & mTransform, "key=b");
	mParams.addButton("Clear", std::bind(& BrushApp::clearPoints, this), "key=c");
	mParams.addSeparator("");
	mParams.addParam("Frame rate", & mFrameRate, "", true);
	mParams.addParam("Full screen", & mFullScreen, "key=d");
	mParams.addButton("Save screen shot", std::bind(& BrushApp::screenShot, this), "key=space");
	mParams.addButton("Quit", std::bind(& BrushApp::quit, this), "key=esc");

	// Run the first resize event to set up OpenGL and point list
	resize(ResizeEvent(getWindowSize()));

}

// This routine cleans up the application as it is exiting
void BrushApp::shutdown()
{

	// Clean up
	mPoints.clear();
	if (mShader)
		mShader.reset();
	if (mShaderPassThru)
		mShaderPassThru.reset();
	if (mShaderTransform)
		mShaderTransform.reset();
	if (mTexture)
		mTexture.reset();
	

}

// This method borrows the term "trace" from ActionScript.
// This is a convenience method for writing a line to the
// console and (on Windows) debug window
void BrushApp::trace(const string & message)
{

	// Write to console and debug window
	console() << message << "\n";
#ifdef CINDER_MSW
	OutputDebugStringA((message + "\n").c_str());
#endif

}

// Runs update logic
void BrushApp::update()
{

	// Update frame rate and elapsed time
	mElapsedFrames = (float)getElapsedFrames();
	mElapsedSeconds = (float)getElapsedSeconds();
	mFrameRate = getAverageFps();

	// Toggle fullscreen mode
	if (mFullScreen != mFullScreenPrev)
	{
		setFullScreen(mFullScreen);
		mFullScreenPrev = mFullScreen;
	}

	// Toggle between transform and pass-thru shader
	if (mTransform != mTransformPrev)
	{
		mShader = mTransform ? mShaderTransform : mShaderPassThru;
		mTransformPrev = mTransform;
	}

}

// Run application
CINDER_APP_BASIC(BrushApp, RendererGl)
