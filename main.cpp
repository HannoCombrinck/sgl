#include <iostream>
#include <future>
#include "myglfw.h"
#include "Timer.h"

using namespace std;

// GPU jobs
/*
	Create VBO, IBO, VAO, shader, texture, framebufferobject etc..
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

// View
/* Information necessary to populate some target buffer (on or off screen)
	Render target (Back buffer or render texture)
	Resolution (defined by render target?)	
	Camera
		View matrix
		Projection matrix

	Layer (Opague, Transparent, Effects, Custom etc.)
	...

*/

typedef unsigned int uStateKey;

class AssetLoader
{
public:
		AssetLoader() {}
		~AssetLoader() {}
		
		shared_ptr<Texture> loadTexture(const string& sFilename) { return null_ptr; /*TODO*/ }
		shared_ptr<Sound> loadSound(const string& sFilename) { return null_ptr; /*TODO*/ }
		//etc.

private:
		unordered_map<string, shared_ptr<Texture>> m_mapTextures;
		unordered_map<string, shared_ptr<Sound>> m_mapSounds;
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

	void setView(const Mat4& m) { m_mView = m; }
	Mat4& modifyView() { return m_mView; }
	const Mat4& getView() const { return m_mView; }

	void setProj(const Mat4& m) { m_mProj = m; }
	Mat4& modifyProj() { return m_mProj; }
	const Mat4& getProj() const { return m_mProj; }

private:
	Mat4 m_mView;
	Mat4 m_mProj;
// position vec
// orientation quat	
};

class View
{
public:
	enum ELayer
	{
		LAYER_OPAQUE = 0,
		LAYER_TRANSPARENT,
		LAYER_COUNT
	};

	View() {}
	~View() {}

private:
	uint m_uTarget;
	uint m_uWidth;
	uint m_uHeight;
	ELayer m_eLayer;	
	shared_ptr<Camera> m_spCamera;
};


class PipelineState
{
public:
	PipelineState() {}
	~PipelineState() {}

private:
	uint m_uTarget;
	uint m_uMaterial; 

};

class DrawCallData
{
public:

private:
	PipelineState m_State;
};

typedef pair<uStateKey, uDrawCallOffset> drawCall;

vector<drawCall>* aDrawCalls_ut;
vector<drawCall>* aDrawCalls_rt;
vector<DrawCallData>* aDrawCallDataBuffer_ut; // Fixed size, write only, buffer to accommodate max number of draw calls 
vector<DrawCallData>* aDrawCallDataBuffer_rt; // Fixed size, read only, buffer to accommodate max number of draw calls 
uint uDrawCallCount;

bool drawCallCompare(const drawCall& d1, const drawCall& d2)
{
	return d1.first < d2.first;	
}

void update(double dt)
{
	aDrawCalls_ut->clear();

	// Traverse scene graph and populate aDrawCalls_ut (with visible geometry) in update thread
	for (const auto& g : m_aGameObjects)
	{
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
void syncMain()
{
	// join update thread
	swap(aDrawCallDataBuffer_ut, aDrawCallDataBuffer_rt); // rather copy than swap?
	// copy RenderJob results from render thread to udpate thread buffer
	// launch update thread for next frame and let this thread continue to render stuff
}

void updateState(const PipelineState& s)
{
	
}

void submit(const DrawCallData& d)
{
	setTarget(d.getState().getTarget());
	setShaders(d.getState().getMaterial());
	setShaderData(d.getState().getMaterial());
	setBuffers(d.getBuffers());
	draw(d.getDrawParams());
}

void render()
{
	// Submit pre-sorted draw calls
	for (const auto& d : aDrawCalls)
	{
		submit(d.second.pDrawCall);
	}
}

int doSomeWork(int iCount)
{
	int iTotal = 0;
	for (int i = 0; i < iCount; ++i)
	{
		iTotal += i % 10;
	}
	return iTotal;
}

int main(int argc, char** argv)
{
	GLWindow window("OpenGL Window", 800, 600, false);
	window.registerKeyPress(GLFW_KEY_A, []() { cout << doSomeWork(10) << endl; });


	auto dCurrentTime = glfwGetTime();
	double dPreviousTime = 0.0;
	double dt = 0.0;
	
	Camera mainCam;
	View fpsView;
	fpsView

	while (window.isRunning())
	{
		// Update thread
		dPreviousTime = dCurrentTime;
		dCurrentTime = glfwGetTime();
		dt = dCurrentTime - dPreviousTime;
		window.processEvents();
		update(dt);

		// Render thread
		render();
		window.swapBuffers();
	}

	return 0; 
}

