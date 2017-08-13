#pragma once

#include <iostream>
#include <assert.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace std;

class GLWindow
{
public:
	GLWindow(const string& sName, int iWidth, int iHeight, bool bFullscreen, int iMajorGLVersion = 3, int iMinorGLVersion = 2)
	{
		glfwSetErrorCallback(errorCallback);
		if (!glfwInit())
		{
			cout << "GLFW: Init error\n";
			assert(false);
		}

		GLFWmonitor *pMonitor = NULL;
		if (bFullscreen)
			pMonitor = glfwGetPrimaryMonitor();

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, iMajorGLVersion);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, iMinorGLVersion);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // TODO: Test forward compatible vs not forward compatible
															 //glfwWindowHint(GLFW_DECORATED, GL_FALSE);

		m_sName = sName;
		m_iWidth = iWidth;
		m_iHeight = iHeight;

		m_pWindow = glfwCreateWindow(m_iWidth, m_iHeight, m_sName.c_str(), pMonitor, NULL);

		if (!m_pWindow)
		{
			glfwTerminate();
			cout << "GLFW: Failed to create window\n";
			assert(false);
		}

		glfwMakeContextCurrent(m_pWindow);

		// Initialize GL extension wrangler
		glewExperimental = true;
		auto eError = glewInit();
		if (eError != GLEW_OK)
		{
			cout << "GLFW: Failed to initialize - " << glewGetErrorString(eError) << endl;
			assert(false);
		}

		m_bIsRunning = true;

		// Setup event callbacks
		/*glfwSetKeyCallback(m_pWindow, keyEventCallback);
		glfwSetMouseButtonCallback(m_pWindow, mouseButtonCallback);
		glfwSetCursorEnterCallback(m_pWindow, mouseEnterCallback);
		glfwSetScrollCallback(m_pWindow, mouseScrollCallback);
		glfwSetWindowPosCallback(m_pWindow, windowMoveCallback);
		glfwSetWindowSizeCallback(m_pWindow, windowResizeCallback);
		glfwSetFramebufferSizeCallback(m_pWindow, windowFrameBufferResizeCallback);
		glfwSetWindowRefreshCallback(m_pWindow, windowRefreshCallback);
		glfwSetWindowCloseCallback(m_pWindow, windowCloseCallback);
		glfwSetDropCallback(m_pWindow, fileDropCallback);*/
	}

	~GLWindow()
	{
		if (m_pWindow)
			glfwDestroyWindow(m_pWindow);
		glfwTerminate();
	}

	bool isRunning()
	{
		return m_bIsRunning;
	}

	void processEvents()
	{
		if (glfwWindowShouldClose(m_pWindow))
			m_bIsRunning = false;

		glfwPollEvents();

		//mouseMove();
	}

	void swapBuffers()
	{
		glfwSwapBuffers(m_pWindow);
	}

private:
	static void errorCallback(int iErrorCode, const char* szErrorMessage)
	{
		cout << "GLFW Error: " << iErrorCode << " : " << szErrorMessage << endl;
	}

	GLFWwindow* m_pWindow;
	string m_sName;
	int m_iWidth;
	int m_iHeight;
	bool m_bIsRunning;
};
