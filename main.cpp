#include <iostream>
#include <future>
#include <unordered_map>

#include <gl/glew.h>
#include <SFML/OpenGL.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <glm/glm.hpp>

#include "timer.h"

using namespace std;

typedef unsigned int uint;
typedef unsigned char uchar;
typedef glm::mat4 Mat4;
typedef glm::vec3 Vec3;
typedef glm::vec2 Vec2;

struct NullPtr
{
	template <class T>
	inline operator shared_ptr<T>() const { return shared_ptr<T>(); }

	template <class T>
	inline operator weak_ptr<T>() const { return weak_ptr<T>(); }

	template <class T>
	inline operator T* () const { return 0; }
};

static NullPtr null_ptr;


// GPU jobs
/*
	Create VBO, IBO, VAO, shader, texture, framebufferobject etc..
*/

// Render thread sequence
/*
	Bind back buffer
	Clear back buffer
	for all objects
		upload necessary shader values
		bind textures
		submit gl draw command
*/

// Buffer containing draw call information 
/* Render something: (View: has a frustum and camera i.e view and projection matrices)
	Bind render target
	Bind shaders
		Upload projection matrix 
		Upload view matrix
	Upload uniforms
		Colours, time values etc.
	Bind textures
	Upload world matrix
	Submit draw call

	sorting key that represents all of the above 
*/

// View (Contains high level data that will be used to populate lower level drawcall/command buffers
/* Information necessary to populate some target buffer (on or off screen)
	Render target (Back buffer or render texture)
	Resolution (defined by render target?)	
	Camera
		View matrix
		Projection matrix

	Layer (Opague, Transparent, Effects, Custom etc.)
	...

*/

/* Matrix naming convenction examples:

Direct3D style:
object_to_projection = object_to_world * world_to_view * view_to_projection;
object_to_view = object_to_world * world_to_view;
object_to_projection = object_to_view * view_to_projection; 

OpenGL style:
projection_from_object = projection_from_view * view_from_world * world_from_object
*/


typedef unsigned int StateKey; // Pack bits into this key to use for draw call sorting

class AssetLoader
{
public:
		AssetLoader() {}
		~AssetLoader() {}
		
		//shared_ptr<Texture> loadTexture(const string& sFilename) { return null_ptr; }
		//shared_ptr<Sound> loadSound(const string& sFilename) { return null_ptr; }
		//etc.

private:
		//unordered_map<string, shared_ptr<Texture>> m_mapTextures;
		//unordered_map<string, shared_ptr<Sound>> m_mapSounds;
		//etc.
};

class Target
{
public:
	Target() {}
	~Target() {}

private:
};

class Camera
{
public:
	Camera() {}
	~Camera() {}

	void setViewFromWorld(const Mat4& m) { m_mViewFromWorld = m; }
	Mat4& modifyViewFromWorld() { return m_mViewFromWorld; }
	const Mat4& getViewFromWorlds() const { return m_mViewFromWorld; }

	void setProjFromView(const Mat4& m) { m_mProjFromView = m; }
	Mat4& modifyProjFromView() { return m_mProjFromView; }
	const Mat4& getProjFromView() const { return m_mProjFromView; }

private:
	Mat4 m_mViewFromWorld;
	Mat4 m_mProjFromView;
// position vec
// orientation quat	
};

class View
{
public:
	enum ELayer {
		LAYER_OPAQUE = 0,
		LAYER_TRANSPARENT,
		LAYER_COUNT
	};

	View(const shared_ptr<Camera>& spCamera)
		: m_spCamera(spCamera)
	{
	}

	~View() {
	}

	void setCamera(const shared_ptr<Camera>& spCamera) {
		m_spCamera = spCamera;
	}

	shared_ptr<Camera> getCamera() const {
		return m_spCamera;
	}

private:
	uint m_uTarget;
	uint m_uWidth;
	uint m_uHeight;
	ELayer m_eLayer;	
	shared_ptr<Camera> m_spCamera;
};


/*class PipelineState
{
public:
	PipelineState() {}
	~PipelineState() {}

private:
	uint m_uTarget;
	uint m_uMaterial; 
};*/

/*class DrawCallData
{
public:

private:
	uint m_uVAO; // Encapsulates vertex buffer and index buffer
	PipelineState m_State;
};*/

enum ECommandType
{
	COMMAND_INVALID			  = 0,
	COMMAND_SET_RENDER_TARGET = 1 << 0,
	COMMAND_SET_VIEWPORT	  = 1 << 1,
	COMMAND_CLEAR			  = 1 << 2,
	COMMAND_DRAW_CALL		  = 1 << 3,
};

struct RenderTargetData
{
	uint uNumTargets;
	uint aTargets[8];
};

struct ViewportData
{
	uint uWidth;
	uint uHeight;
	float fNear;
	float fFar;
};

struct ClearData
{
	uint uFlags;
};

struct DrawCallData
{
	// Temp example data:
	uint uHandle1;
	uint uHandle2;
	Mat4 mWorldFromModel;
};

struct Command
{
	Command() {}

	void tempExecute()
	{
		switch (m_eType)
		{
		case COMMAND_SET_RENDER_TARGET: 
		{
			auto pRenderTargetData = reinterpret_cast<RenderTargetData*>(aData);
			// TODO: Use pRenderTargetData
			break;
		}
		case COMMAND_SET_VIEWPORT:
		{
			auto pViewportData = reinterpret_cast<ViewportData*>(aData);
			// TODO: Use pViewportData
			break;
		}
		case COMMAND_CLEAR:
		{
			auto pClearData = reinterpret_cast<ClearData*>(aData);
			// TODO: Use pClearData
			break;
		}
		case COMMAND_DRAW_CALL:
		{
			auto pDrawCallData = reinterpret_cast<DrawCallData*>(aData);
			// Use pDrawCallData contents to submit draw call to driver
			break;
		}
		default:
			cout << "Invalid command type\n";
			assert(false);
			break;
		}
		
	}

	ECommandType m_eType;
	uchar aData[500]; // Number of bytes required to represent draw command state - calculate based on DrawCallData size
};

typedef pair<StateKey, uint> drawCall;

vector<drawCall>* aDrawCalls_ut;
vector<drawCall>* aDrawCalls_rt;
vector<DrawCallData>* aDrawCallDataBuffer_ut; // Fixed size, write only, buffer to accommodate max number of draw calls 
vector<DrawCallData>* aDrawCallDataBuffer_rt; // Fixed size, read only, buffer to accommodate max number of draw calls 
uint uDrawCallCount;

bool drawCallCompare(const drawCall& d1, const drawCall& d2) {
	return d1.first < d2.first;	
}

/*
void update(double dt) {
	aDrawCalls_ut->clear();

	// Traverse scene graph and populate aDrawCalls_ut (with visible geometry) in update thread
	for (const auto& g : m_aGameObjects) {
		// Update game object
		//	- this update potentially modifies contents of entries in draw call data buffer
		//	- if this is the then mark the draw call data entry as "modified" and update
		//	  min and max buffer offset values.
		//	  This will be used to only copy parts of the buffer that has changed.

		auto aDC = g.getVisual().getDrawCalls();
		// TODO: iterate aDC and get min and max offsets of "modified" draw call data
		// 	 later on only the the parts of DrawCallDataBuffer that has changed will be copied
		aDrawCalls.insert(aDrawCalls.end(), aDC.begin(), aDC.end());
	}
	sort(aDrawCalls, drawCallCompare); // Should sort happen in update thread or render thread?

	syncMain();		
}

// Synchronize update and render threads and copy update frame data results render frame data
void syncMain() {
	// join update thread
	swap(aDrawCallDataBuffer_ut, aDrawCallDataBuffer_rt); // rather copy than swap?
	// copy RenderJob results from render thread to udpate thread buffer
	// launch update thread for next frame and let this thread continue to render stuff
}

void updateState(const PipelineState& s) {
	
}

void submit(const DrawCallData& d) {
	setTarget(d.getState().getTarget());
	setShaders(d.getState().getMaterial());
	setShaderData(d.getState().getMaterial());
	setBuffers(d.getBuffers());
	draw(d.getDrawParams());
}

void render() {
	// Submit pre-sorted draw calls
	for (const auto& d : aDrawCalls)
	{
		submit(d.second.pDrawCall);
	}
}
*/
///////////////////////////////////////////////////////////////////////////////////////////

// Create a window with an OpenGL context that handles system events (including user input)
class GLWindow
{
public:
	GLWindow(const string& sName, uint uWidth, uint uHeight)
		: m_bRunning(true)
	{
		sf::ContextSettings settingsRequested;
		settingsRequested.depthBits = 24;
		settingsRequested.stencilBits = 8;
		settingsRequested.antialiasingLevel = 4;
		m_pWindow = unique_ptr<sf::Window>(new sf::Window(sf::VideoMode(uWidth, uHeight), sName.c_str(), sf::Style::Default, settingsRequested));
		m_pWindow->setVerticalSyncEnabled(true);
		m_pWindow->setActive(true); // This call set the OpenGL context as well - only thread with active window can call OpenGL calls
	
		glewExperimental = true;
		auto eError = glewInit();
		if (eError != GLEW_OK)
		{
			cout << "Failed to initialize glew\n";
			assert(false);
		}

		auto settingsUsed = m_pWindow->getSettings();
		cout << "Created window with OpenGL context version: " << settingsUsed.majorVersion << "." << settingsUsed.minorVersion << endl;
	}

	~GLWindow() {
		// Do SFML cleanup before m_pWindow is destroyed 
	}

	bool isRunning() const { 
		return m_bRunning; 
	}

	double getDeltaTime() {
		m_Time = m_Clock.getElapsedTime();
		m_Clock.restart();
		return double(m_Time.asSeconds());
	}

	void processEvents() {
		while (m_pWindow->pollEvent(m_Event)) {
			if (m_Event.type == sf::Event::Closed)
			{
				m_bRunning = false;
			}
			else if (m_Event.type == sf::Event::Resized)
			{
				m_uWidth = m_Event.size.width;
				m_uHeight = m_Event.size.height;
				glViewport(0, 0, m_uWidth, m_uHeight); // Move to View
			}
			else if (m_Event.type == sf::Event::KeyPressed)
			{
				switch (m_Event.key.code)
				{
				case sf::Keyboard::T:
					cout << "Last delta time: " << m_Time.asMilliseconds() << " ms\n";
					break;
				}
			}
			// TODO: Handle other events
		}
	}

	void swapBuffers() {
		m_pWindow->display(); // if SFML is set to enable v-sync this display() call blocks until next v-sync happens 
	}

private:
	unique_ptr<sf::Window> m_pWindow;
	sf::Event m_Event;
	sf::Time m_Time;
	sf::Clock m_Clock;
	uint m_uWidth;
	uint m_uHeight;
	bool m_bRunning;
};

struct SVertex
{
	Vec3 vPos;
	Vec2 vUV;
};

uint uVAO = ~0;
uint uVBO = ~0;
uint uIB = ~0;

void loadAssets()
{
	vector<SVertex> aVertices = {
		{ Vec3(0.0f, 0.0f, 0.0f), Vec2(0.0, 0.0) }, // Vertex 0
		{ Vec3(0.5f, 0.0f, 0.0f), Vec2(1.0, 0.0) }, // Vertex 1
		{ Vec3(0.5f, 0.5f, 0.0f), Vec2(1.0, 1.0) }, // Vertex 2
		{ Vec3(0.0f, 0.5f, 0.0f), Vec2(0.0, 1.0) }  // Vertex 3
	};

	vector<uint> aIndices = {
		0, 1, 2, // Triangle 0
		0, 2, 3  // Triangle 1
	};

	// Create vertex array object
	glGenVertexArrays(1, &uVAO);
	glBindVertexArray(uVAO);

	// Create vertex buffer object
	glGenBuffers(1, &uVBO);
	glBindBuffer(GL_ARRAY_BUFFER, uVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(SVertex) * aVertices.size(), reinterpret_cast<void*>(&aVertices[0]), GL_STATIC_DRAW);

	// Set vertex attribute layouts
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SVertex), (GLvoid*) 0); // Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, true, sizeof(SVertex), (GLvoid*) 3); // Texture coordinate attribute
	glEnableVertexAttribArray(1);

	// Create index buffer object
	glGenBuffers(1, &uIB);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, uIB);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint) * aIndices.size(), reinterpret_cast<void*>(&aIndices[0]), GL_STATIC_DRAW);

	glBindVertexArray(0);
	

	// TODO: Create texture
}

void cleanupAssets()
{
	glDeleteBuffers(1, &uVBO);
	glDeleteBuffers(1, &uIB);
	glDeleteBuffers(1, &uVAO);
}

int main(int argc, char** argv) {
	GLWindow window("OpenGL Window", 800, 600);

	loadAssets();

	auto spFpsCam = make_shared<Camera>();
	View fpsView(spFpsCam);

	while (window.isRunning()) {
		// Update thread
		window.processEvents();
		auto dt = window.getDeltaTime();
		//update(dt);
		glMatrixMode(GL_MODELVIEW);
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f);


		// Render thread
		// render();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(uVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const GLvoid*) 0);
		glBindVertexArray(0);
		
		window.swapBuffers();
	}

	cleanupAssets();

	return 0;
}

