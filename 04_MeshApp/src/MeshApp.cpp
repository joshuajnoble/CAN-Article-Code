// Includes
#include <cinder/app/AppBasic.h>
#include <cinder/Arcball.h>
#include <cinder/Camera.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Fbo.h>
#include <cinder/gl/Vbo.h>
#include <cinder/ImageIo.h>
#include <cinder/params/Params.h>
#include <cinder/Utilities.h>
#include "Resources.h"

/*
 * This application demonstrates how to create and update a
 * frame buffer object. Then, using GLSL, we'll read the color
 * data to create a solid, 3D mesh out of a vertex buffer object.
 * We're using a frame buffer object as the source data so we can
 * can read the values of each vertex's neighbor, which is 
 * impossible with a VBO alone.
 */

// GPU mesh
class MeshApp : public ci::app::AppBasic 
{

public:

	// Cinder callbacks
	void draw();
	void mouseDown(ci::app::MouseEvent event);
	void mouseDrag(ci::app::MouseEvent event);
	void mouseMove(ci::app::MouseEvent event);
	void mouseWheel(ci::app::MouseEvent event);
	void prepareSettings(ci::app::AppBasic::Settings * settings);
	void resize();
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

	// Shader
	void loadShaders();
	ci::gl::GlslProg mFboShader;
	double mGlslVersion;
	bool mTransform;
	bool mTransformPrev;
	ci::gl::GlslProg mVboShader;
	ci::gl::GlslProg mVboShaderPassThru;
	ci::gl::GlslProg mVboShaderTransform;

	// Lighting
	ci::ColorAf mLightAmbient;
	ci::ColorAf mLightDiffuse;
	ci::Vec3f mLightPosition;
	float mLightShininess;
	ci::ColorAf mLightSpecular;

	// FBO
	GLenum mColorAttachment[1];
	bool mDrawFbo;
	ci::gl::Fbo mFbo;
	ci::gl::Fbo::Format mFboFormat;
	ci::Surface32f mSurfacePosition;
	ci::gl::Texture::Format mTextureFormat;
	ci::gl::Texture	mTexturePosition;
	
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
	void screenShot();
	void trace(const std::string & message);

};

// Import namespaces
using namespace ci;
using namespace ci::app;
using namespace std;

// Renders the scene
void MeshApp::draw()
{

	// Set up the scene
	gl::clear(mBackgroundColor, true);
	gl::setViewport(getWindowBounds());

	// By setting mDrawFbo to true, we can see what
	// is happening to the FBO
	if (mDrawFbo)
	{
		gl::setMatricesWindow(getWindowSize(), true);
		mFbo.getTexture().enableAndBind();
		gl::draw(mFbo.getTexture(), Area(getWindowSize().x / 4, getWindowSize().y / 4, 3 * (getWindowSize().x / 4), 3 * (getWindowSize().y / 4)));
		mFbo.getTexture().unbind();
	}

	// Set matrices to camera view
	gl::setMatrices(mCamera);

	// Take the FBO to which we rendered in ::update()
	// and bind its color attachment as a texture
	mFbo.bindTexture(0, 0);

	// Bind the shader to render the VBO
	mVboShader.bind();
	
	// Move into position
	gl::pushModelView();
	gl::translate(mMeshOffset);
	gl::rotate(mMeshRotation);

	// Set shader uniforms
	mVboShader.uniform("alpha", mMeshAlpha);
	mVboShader.uniform("eyePoint", mEyePoint);
	mVboShader.uniform("lightAmbient", mLightAmbient);
	mVboShader.uniform("lightDiffuse", mLightDiffuse);
	mVboShader.uniform("lightPosition", mLightPosition);
	mVboShader.uniform("lightSpecular", mLightSpecular);
	mVboShader.uniform("mvp", gl::getProjection() * gl::getModelView());
	mVboShader.uniform("pixel", Vec2f(1.0f / (float)mMeshWidth, 1.0f / (float)mMeshHeight));
	mVboShader.uniform("positions", 0);
	mVboShader.uniform("shininess", mLightShininess);
	mVboShader.uniform("transform", mTransform);
	mVboShader.uniform("uvmix", mMeshUvMix);
	
	// Draw VBO
	gl::draw(mVboMesh);

	// Stop drawing
	gl::popModelView();
	mVboShader.unbind();
	mFbo.unbindTexture();

	// Draw the params interface
	mParams.draw();

}

// This routine creates a frame buffer object which we will
// use as a depth map for the vertex buffer object. The VBO is 
// basically just a grid of evenly spaced vertices (points). By
// using a FBO for the source, the shaders will be able to see the
// values of each point's neighbor so we can form a solid mesh.
void MeshApp::initMesh()
{

	/* STEP 1
	 * 
	 * Create a surface and set the color data for each pixel.
	 * The surfaces will hold XYZ coordinates in the RGB values.
	 */

	// Clear surfaces if they already exist
	if (mSurfacePosition)
		mSurfacePosition.reset();

	// Create empty surfaces to hold position data
	mSurfacePosition = Surface32f(mMeshWidth, mMeshHeight, true, SurfaceChannelOrder::RGBA);

	// Iterate through the position surface's pixels.
	// Convert the position to world space and set the 
	// position data in the color of each pixel.
	Surface32f::Iter pixelIt = mSurfacePosition.getIter();
	while (pixelIt.line())
		while (pixelIt.pixel())
			mSurfacePosition.setPixel(pixelIt.getPos(), ColorAf(
				(float)pixelIt.x() * 2.0f - (float)mMeshWidth, 
				(float)pixelIt.y() * 2.0f - (float)mMeshHeight, 
				0.0f, 1.0f));

	/* STEP 2
	 * 
	 * With the surface data populated, we will now create a texture.
	 * The difference between a surface and texture is that a 
	 * surface is computed by the CPU. A texture is uploaded to the 
	 * GPU and is processed or displayed there, which is much faster.
	 */

	// Clear texture if it already exists
	if (mTexturePosition)
		mTexturePosition.reset();

	// Create position texture
	mTexturePosition = gl::Texture(mSurfacePosition, mTextureFormat);
	mTexturePosition.setWrap(GL_REPEAT, GL_REPEAT);
	mTexturePosition.setMinFilter(GL_NEAREST);
	mTexturePosition.setMagFilter(GL_NEAREST);

	/* STEP 3
	 * 
	 * Now we're going to create the frame buffer object. The idea
	 * is to send the position texture to the GPU, then draw it onto 
	 * the FBO using a shader to manipulate the color data.
	 */

	// Clear current FBO, if needed
	if (mFbo)
		mFbo.reset();

	// Initialize FBO
	mFbo = gl::Fbo(mMeshWidth, mMeshHeight, mFboFormat);

	// Bind the FBO so we can draw onto it
	mFbo.bindFramebuffer();
	
	// The FBO has a color attachment buffer. We use this instead
	// of the standard buffer because we can use a larger range 
	// of values.
	glDrawBuffer(mColorAttachment[0]);

	// Set up the render target
	gl::clear(ColorAf::black(), true);
	gl::setViewport(mFbo.getBounds());
	gl::setMatricesWindow(mFbo.getSize(), false);

	// Draw the position texture into the FBO's color attachment
	mTexturePosition.enableAndBind();
	gl::draw(mTexturePosition, mFbo.getBounds());
	mTexturePosition.unbind();

	// Unbind the FBO to change our render target to the screen
	mFbo.unbindFramebuffer();

	/* STEP 4
	 * 
	 * The final step in creating our mesh is to make a vertex
	 * buffer object. This holds the actual 3D data that we will 
	 * render to the screen.
	 */

	// Clear the VBO if it already exists
	if (mVboMesh)
		mVboMesh.reset();

	// Iterate through the mesh dimensions
	for (int32_t y = 0; y < mMeshHeight; y++)
		for (int32_t x = 0; x < mMeshWidth; x++)
		{

			// Set the index of the vertex in the VBO so it is
			// numbered left to right, top to bottom
			mVboIndices.push_back(x + y * mMeshWidth);

			// Set the position of the vertex in world space
			mVboVertices.push_back(Vec3f((float)x - (float)mMeshWidth * 0.5f, (float)y - (float)mMeshHeight * 0.5f, 0.0f));

			// Use percentage of the position for the texture coordinate
			mVboTexCoords.push_back(Vec2f((float)x / (float)mMeshWidth, (float)y / (float)mMeshHeight));

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
	resize();

}

// Load GLSL shaders from resources
void MeshApp::loadShaders()
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

			// We're loading the VBO shader into two instances. The first 
			// passes through the geometry shader so you can see what 
			// the VBO looks like without the transformation. The 
			// second converts points to quads on the GPU. Note that we
			// set the number of output vertices to 1 for GL_POINTS.
			mVboShaderPassThru = gl::GlslProg(
				loadResource(RES_SHADER_VBO_VERT_150), 
				loadResource(RES_SHADER_VBO_FRAG_150), 
				loadResource(RES_SHADER_VBO_GEOM_150), 
				GL_POINTS, GL_POINTS, 1
				);
			mVboShaderTransform = gl::GlslProg(
				loadResource(RES_SHADER_VBO_VERT_150), 
				loadResource(RES_SHADER_VBO_FRAG_150), 
				loadResource(RES_SHADER_VBO_GEOM_150), 
				GL_POINTS, GL_TRIANGLE_STRIP, maxGeomOutputVertices
				);

			// Load the FBO shader
			mFboShader = gl::GlslProg(
				loadResource(RES_SHADER_FBO_VERT_150), 
				loadResource(RES_SHADER_FBO_FRAG_150)
				);

		}
		else
		{
            
			// Same as above, but these are GLSL version 1.2 shaders.
			// These are a bit slower, but should work with older systems.
			mVboShaderPassThru = gl::GlslProg(
				loadResource(RES_SHADER_VBO_VERT_120), 
				loadResource(RES_SHADER_VBO_FRAG_120), 
				loadResource(RES_SHADER_VBO_GEOM_120), 
				GL_POINTS, GL_POINTS, 1
				);
			mVboShaderTransform = gl::GlslProg(
				loadResource(RES_SHADER_VBO_VERT_120), 
				loadResource(RES_SHADER_VBO_FRAG_120), 
				loadResource(RES_SHADER_VBO_GEOM_120), 
				GL_POINTS, GL_TRIANGLE_STRIP, maxGeomOutputVertices
            );
            
            // Load the FBO shader
			mFboShader = gl::GlslProg(
                loadResource(RES_SHADER_FBO_VERT_120), 
                loadResource(RES_SHADER_FBO_FRAG_120)
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
	mVboShader = mVboShaderTransform;

}

// Handles mouse down event
void MeshApp::mouseDown(MouseEvent event)
{

	// Set cursor position
	mCursor = event.getPos();

	// Update arcball, set camera rotation if holding ALT
	mArcball.mouseDown(event.getPos());
	if (event.isAltDown())
		mMeshRotation = mArcball.getQuat().v;

}

// Handles mouse drag event
void MeshApp::mouseDrag(MouseEvent event)
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
void MeshApp::mouseMove(MouseEvent event)
{

	// Set cursor position
	mCursor = event.getPos();

}

// Handles mouse wheel
void MeshApp::mouseWheel(MouseEvent event)
{

	// Move the mesh along the Z axis using the
	// mouse wheel
	mMeshOffset.z -= event.getWheelIncrement();

}

// Prepare window settings
void MeshApp::prepareSettings(ci::app::AppBasic::Settings * settings)
{

	// Set up window
	settings->setTitle("MeshApp");
	settings->setWindowSize(1024, 600);
	settings->setFrameRate(60.0f);
	settings->setFullScreen(false);

}

// Handles window resize event
void MeshApp::resize()
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
void MeshApp::screenShot()
{

	// Save PNG of screen to application folder
	//writeImage(fs::path(getAppPath().generic_string() + "frame" + toString(getElapsedFrames()) + ".png"), copyWindowSurface());

}

// Set up
void MeshApp::setup()
{

	// Load the shader
	loadShaders();

	// Enable geometry transformation
	mTransform = true;
	mTransformPrev = mTransform;

	// Set VBO layout
	mVboLayout.setStaticIndices();
	mVboLayout.setStaticPositions();
	mVboLayout.setStaticTexCoords2d();

	// Set up FBO format
	mFboFormat.enableDepthBuffer(false);
	mFboFormat.enableColorBuffer(true, 1);
	mFboFormat.setMinFilter(GL_NEAREST);
	mFboFormat.setMagFilter(GL_NEAREST);
	mFboFormat.setColorInternalFormat(GL_RGBA_FLOAT32_ATI);
	
	// Set color attachment buffer. We're only using one
	// attachment, but it needs to be held in an array.
	mColorAttachment[0] = GL_COLOR_ATTACHMENT0_EXT;

	// Set up the texture format for 
	mTextureFormat.setInternalFormat(GL_RGBA_FLOAT32_ATI);

	// Intialize camera
	mEyePoint = Vec3f::zero();
	mLookAt = Vec3f::zero();

	// Initialize cursor position
	mCursor = Vec2i::zero();

	// Set background color
	mBackgroundColor = Colorf(0.415f, 0.434f, 0.508f);
	
	// Set this to true in the parameters to show the FBO
	mDrawFbo = false;

	// Set fullscreen flag for parameters
	mFullScreen = isFullScreen();
	mFullScreenPrev = mFullScreen;
	
	// Set default mesh dimensions
	mMeshAlpha = 0.8f;
	mMeshHeight = 128;
	mMeshHeightPrev = mMeshHeight;
	mMeshOffset = Vec3f::zero();
	mMeshRotation = Vec3f(75.0f, 0.0f, 330.0f);
	mMeshScale = 1.5f;
	mMeshUvMix = 0.133f;
	mMeshWaveAmplitude = 12.0f;
	mMeshWaveSpeed = 1.0f;
	mMeshWaveWidth = 0.04f;
	mMeshWidth = 128;
	mMeshWidthPrev = mMeshWidth;

	// Set up the light. This application does not actually use OpenGL 
	// lighting. Instead, it passes a light position and color 
	// values to the shader. Per fragment lighting is calculated in GLSL.
	mLightAmbient = ColorAf(0.5f, 0.5f, 0.5f, 1.0f);
	mLightDiffuse = ColorAf(0.25f, 0.25f, 0.25f, 1.0f);
	mLightPosition = Vec3f(-270.0f, -340.0f, -300.0f);
	mLightShininess = 20.0f;
	mLightSpecular = ColorAf(0.75f, 0.75f, 0.75f, 1.0f);

	// Create the parameters bar
	mParams = params::InterfaceGl("Parameters", Vec2i(250, 390));
	mParams.addSeparator("");
	mParams.addText("Hold ALT to rotate");
	mParams.addText("Hold SHIFT to drag");
	mParams.addText("Wheel to zoom");
	mParams.addSeparator("");
	mParams.addParam("Mesh transform enabled", & mTransform, "key=a");
	mParams.addParam("Mesh height", & mMeshHeight, "min=1 max=2048 step=1 keyDecr=b keyIncr=B");
	mParams.addParam("Mesh width", & mMeshWidth, "min=1 max=2048 step=1 keyDecr=c keyIncr=C");
	mParams.addParam("Mesh scale", & mMeshScale, "min=0.1 max=30000.0 step=0.1 keyDecr=d keyIncr=D");
	mParams.addParam("Mesh wave amplitude", & mMeshWaveAmplitude, "min=0.000 max=30000.000 step=0.001 keyDecr=e keyIncr=E");
	mParams.addParam("Mesh wave speed", & mMeshWaveSpeed, "min=0.000 max=100.000 step=0.001 keyDecr=f keyIncr=F");
	mParams.addParam("Mesh wave width", & mMeshWaveWidth, "min=0.000 max=30000.000 step=0.001 keyDecr=g keyIncr=G");
	mParams.addParam("Show FBO", & mDrawFbo, "key=h");
	mParams.addSeparator("");
	mParams.addParam("Light position", & mLightPosition);
	mParams.addSeparator("");
	mParams.addParam("Frame rate", & mFrameRate, "", true);
	mParams.addParam("Full screen", & mFullScreen, "key=i");
	mParams.addButton("Save screen shot", std::bind(& MeshApp::screenShot, this), "key=space");
	mParams.addButton("Quit", std::bind(& MeshApp::quit, this), "key=esc");
	
	// Create mesh
	initMesh();

}

// This routine cleans up the application as it is exiting
void MeshApp::shutdown()
{

	// Clean up
	if (mFbo)
		mFbo.reset();
	if (mFboShader)
		mFboShader.reset();
	if (mSurfacePosition)
		mSurfacePosition.reset();
	if (mTexturePosition)
		mTexturePosition.reset();
	mVboIndices.clear();
	if (mVboMesh)
		mVboMesh.reset();
	if (mVboShader)
		mVboShader.reset();
	if (mVboShaderPassThru)
		mVboShaderPassThru.reset();
	if (mVboShaderTransform)
		mVboShaderTransform.reset();
	mVboTexCoords.clear();
	mVboVertices.clear();

}

// This method borrows the term "trace" from ActionScript.
// This is a convenience method for writing a line to the
// console and (on Windows) debug window
void MeshApp::trace(const string & message)
{

	// Write to console and debug window
	console() << message << "\n";
#ifdef CINDER_MSW
	OutputDebugStringA((message + "\n").c_str());
#endif

}

// Runs update logic
void MeshApp::update()
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
	if (mTransform != mTransformPrev)
	{
		mVboShader = mTransform ? mVboShaderTransform : mVboShaderPassThru;
		mTransformPrev = mTransform;
	}

	// Update camera
	mCamera.lookAt(mEyePoint, mLookAt);

	/*
	 * In the routine below, we'll bind the FBO as our
	 * render target, and the position texture as an input.
	 * Using time and some other parameters, along with the 
	 * input texture, the shader will draw onto the FBO.
	 */

	// Set up the window to render to the FBO
	gl::setMatricesWindow(mFbo.getSize(), false);
	gl::setViewport(mFbo.getBounds());
	
	// Bind the FBO to draw on it
	mFbo.bindFramebuffer();

	// This tells OpenGL that we're going to render to 
	// the color attachment
	glDrawBuffers(1, mColorAttachment);

	// Bind the position texture as the data source
	mTexturePosition.bind(0);

	// Bind and configure the FBO shader
	mFboShader.bind();
	mFboShader.uniform("amp", mMeshWaveAmplitude);
	mFboShader.uniform("alpha", mMeshAlpha);
	mFboShader.uniform("phase", mElapsedSeconds);
	mFboShader.uniform("positions", 0);
	mFboShader.uniform("scale", mMeshScale);
	mFboShader.uniform("speed", mMeshWaveSpeed);
	mFboShader.uniform("width", mMeshWaveWidth);

	// Draw a quad at the texture's size to process the FBO.
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.0f, 0.0f);
		gl::vertex(0.0f, 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		gl::vertex(0.0f, (float)mMeshHeight);
		glTexCoord2f(1.0f, 1.0f);
		gl::vertex((float)mMeshWidth, (float)mMeshHeight);
		glTexCoord2f(1.0f, 0.0f);
		gl::vertex((float)mMeshWidth, 0.0f);
	}
	glEnd();
	
	// Unbind the shader
	mFboShader.unbind();

	// Unbind the input texture
	mTexturePosition.unbind();
	
	// Unbind the render target
	mFbo.unbindFramebuffer();

}

// Run application
CINDER_APP_BASIC(MeshApp, RendererGl)
