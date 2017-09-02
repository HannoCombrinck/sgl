#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
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
	COMMAND_INVALID = 0,
	COMMAND_SET_RENDER_TARGET = 1 << 0,
	COMMAND_SET_VIEWPORT = 1 << 1,
	COMMAND_CLEAR = 1 << 2,
	COMMAND_DRAW_CALL = 1 << 3,
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
		switch (eType)
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

	ECommandType eType;
	uchar aData[256]; // Number of bytes required to represent draw command state - calculate based on DrawCallData size
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
				case sf::Keyboard::Escape:
					m_bRunning = false;
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
	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(SVertex), (GLvoid*)0); // Position attribute
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, true, sizeof(SVertex), (GLvoid*)3); // Texture coordinate attribute
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

int myRand(int i)
{
	return int(round(rand() / float(RAND_MAX) * (i - 1)));
}

void makeClear(Command* c, uint uFlags)
{
	c->eType = COMMAND_CLEAR;
	auto p = reinterpret_cast<ClearData*>(c->aData);
	p->uFlags = uFlags;
}

void makeSetViewport(Command* c, uint uWidth, uint uHeight, float fNear, float fFar)
{
	c->eType = COMMAND_SET_VIEWPORT;
	auto p = reinterpret_cast<ViewportData*>(c->aData);
	p->uWidth = uWidth;
	p->uHeight = uHeight;
	p->fNear = fNear;
	p->fFar = fFar;
}

void makeDrawCall(Command* c, uint u1, uint u2, const Mat4& m)
{
	c->eType = COMMAND_DRAW_CALL;
	auto p = reinterpret_cast<DrawCallData*>(c->aData);
	p->uHandle1 = u1;
	p->uHandle2 = u2;
	p->mWorldFromModel = m;
}

static const int COUNT = 100000;
Command* aUpdateBuffer;
Command* aRenderBuffer;
Command* aTempBuffer;
int iCommandCount = 0;
uint uResultTotal = 0;
uint uUpdateModifier = 0;

mutex bufferMutex;
condition_variable updateCV;
bool bReadyToUpdate = false; // Additional variable to check for "spurious wakeup" from update thread's side
bool bReadyToRender = false; // Additional variable to check for "spurious wakeup" from main thread's side
// bReadyToUpdate and bReadyToRender is protected under bufferMutex
bool bIsRunning = true;

void runUpdate()
{
	uUpdateModifier++;

	///////
	// This thread will read inputs, run physics, update transforms etc.
	// Fill command buffer with commands to be executed by GPU
	///////

	// Populate "update" buffer 
	Timer t("PopulateTime");
	t.start();
	Command* pCurrentUpdate = aUpdateBuffer;
	int iRand = 0;
	for (int i = 0; i < COUNT; ++i)
	{
		iRand = i % 3;// myRand(3);

		switch (iRand)
		{
		case 0:
		{
			makeClear(pCurrentUpdate, 5U + uUpdateModifier);
			break;
		}
		case 1:
		{
			makeSetViewport(pCurrentUpdate, 800U, 600U, 0.1f, 1000.0f);
			break;
		}
		case 2:
		{
			Mat4 mWorld;
			mWorld[0][2] = 7.0f;
			makeDrawCall(pCurrentUpdate, 3U + uUpdateModifier, 6U + uUpdateModifier, mWorld);
			break;
		}
		default:
			cout << "this shouldn't happen\n";
			break;
		}

		pCurrentUpdate++;
	}
	t.stop();
	//cout << "Populate buffers time:\t" << t.getTime() << "ms\n";

	iCommandCount = COUNT;
}

void runUpdateMT()
{
	while (true)
	{
		// Wait on update condition variable before going further
		unique_lock<mutex> bufferLock(bufferMutex);
		updateCV.wait(bufferLock, []() {return bReadyToUpdate; });

		if (!bIsRunning)
			return;

		runUpdate();

		// Unlock mutex and notify thread waiting on update condition variable
		bReadyToUpdate = false;
		bReadyToRender = true;
		bufferLock.unlock();
		updateCV.notify_one();
	}
}


void swapCommandBuffers()
{
	swap(aUpdateBuffer, aRenderBuffer);
}

void runWork()
{
	Timer t("WorkTime");
	t.start();
	uint uWorkResult = 0;
	// Start populating new "update" buffer in new thread and simultaneously traverse "render" buffer in current thread
	auto pCurrentRender = aRenderBuffer;
	for (int i = 0; i < iCommandCount; ++i)
	{
		switch (pCurrentRender->eType)
		{
		case COMMAND_SET_RENDER_TARGET:
		{
			auto pRenderTargetData = reinterpret_cast<RenderTargetData*>(pCurrentRender->aData);
			// TODO
			break;
		}
		case COMMAND_SET_VIEWPORT:
		{
			auto pViewportData = reinterpret_cast<ViewportData*>(pCurrentRender->aData);
			//cout << i << ") Set Viewport\n";
			uWorkResult += uint(pViewportData->uWidth);
			uWorkResult += uint(pViewportData->uHeight);
			uWorkResult += uint(pViewportData->fNear);
			uWorkResult += uint(pViewportData->fFar);
			break;
		}
		case COMMAND_CLEAR:
		{
			auto pClearData = reinterpret_cast<ClearData*>(pCurrentRender->aData);
			//cout << i << ") Clear\n";
			uWorkResult += uint(pClearData->uFlags);
			break;
		}
		case COMMAND_DRAW_CALL:
		{
			auto pDrawCallData = reinterpret_cast<DrawCallData*>(pCurrentRender->aData);
			//cout << i << ") Draw Call\n";
			uWorkResult += pDrawCallData->uHandle1;
			uWorkResult += pDrawCallData->uHandle2;
			uWorkResult += uint(pDrawCallData->mWorldFromModel[0][2]);
			break;
		}
		default:
			cout << "Invalid command type\n";
			assert(false);
			break;
		}

		pCurrentRender++;
	}
	t.stop();
	uResultTotal += uWorkResult;
	//cout << "Work time:\t\t" << t.getTime() << "ms\n";
	//cout << "Work result:\t\t" << uWorkResult << endl << endl << endl;
}

void startUpdateThread()
{
	unique_lock<mutex> bufferLock(bufferMutex);
	bReadyToUpdate = true;
	updateCV.notify_one(); // updateThread should start running now
}

void syncUpdateThread()
{
	unique_lock<mutex> bufferLock(bufferMutex);
	updateCV.wait(bufferLock, []() { return bReadyToRender; });
}

void terminateUpdateThread()
{
	unique_lock<mutex> bufferLock(bufferMutex);
	bIsRunning = false;
	bReadyToUpdate = true;
	updateCV.notify_one();
}

void testCommands()
{
	Timer ta("MallocTime");
	ta.start();
	aUpdateBuffer = new Command[COUNT];
	aRenderBuffer = new Command[COUNT];
	ta.stop();
	cout << "Malloc time:\t" << ta.getTime() << "ms\n\n";

	int iIterations = 500;


	Timer tTotal("TotalTime");
	tTotal.start();

	uResultTotal = 0;
	
	uUpdateModifier = 0;
	for (int testCount = 0; testCount < iIterations; ++testCount)
	{
		runUpdate();
		swapCommandBuffers();
		runWork();
	}

	tTotal.stop();
	cout << "Result: " << uResultTotal << endl;
	cout << "Total time for everything (single thread): " << tTotal.getTime() << "ms\n\n";

	uResultTotal = 0;
	tTotal.start();

	uUpdateModifier = 0;
	runUpdate();
	for (int testCount = 0; testCount < iIterations; ++testCount)
	{
		swapCommandBuffers();
		thread workThread(runWork);
		runUpdate();
		workThread.join();
	}

	tTotal.stop();
	cout << "Result: " << uResultTotal << endl;
	cout << "Total time for everything (separate threads - new thread every update): " << tTotal.getTime() << "ms\n\n";


	uResultTotal = 0;
	tTotal.start();

	uUpdateModifier = 0;
	thread updateThread(runUpdateMT); // Create update thread
	startUpdateThread(); // Do the first update
	for (int testCount = 0; testCount < iIterations -1; ++testCount)
	{
		syncUpdateThread();
		swapCommandBuffers(); // Neither update or render code should be running here
		startUpdateThread(); // Start update for next frame
		runWork(); // Submit work of current frame
	}
	syncUpdateThread(); // Finish last update
	terminateUpdateThread(); // Terminate update thread
	updateThread.join();
	swapCommandBuffers();
	runWork(); // Submit work for last frame


	tTotal.stop();
	cout << "Result: " << uResultTotal << endl;
	cout << "Total time for everything (separate threads - reusing one thread): " << tTotal.getTime() << "ms\n\n";
	

	Timer td("DeallocTimer");
	td.start();
	delete[] aUpdateBuffer;
	delete[] aRenderBuffer;
	td.stop();
	cout << "Dealloc time: " << td.getTime() << "ms\n";
}

int main(int argc, char** argv) {

	testCommands();

	GLWindow window("OpenGL Window", 800, 600);

	loadAssets();

	auto spFpsCam = make_shared<Camera>();
	View fpsView(spFpsCam);

	double dt = 0.0;
	while (window.isRunning()) {
		// Update thread
		window.processEvents();
		dt = window.getDeltaTime();
		//update(dt);
		glMatrixMode(GL_MODELVIEW);
		glRotatef(1.0f, 0.0f, 0.0f, 1.0f);


		// Render thread
		// render();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(uVAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const GLvoid*)0);
		glBindVertexArray(0);

		window.swapBuffers();
	}

	cleanupAssets();

	return 0;
}

