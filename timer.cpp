#include "timer.h"

#include <windows.h>

Timer::Timer(const std::string& sName)
	: m_sName(sName)
{
}

Timer::~Timer()
{
}

void Timer::start()
{
	LARGE_INTEGER iFrequency;
	LARGE_INTEGER iCount;
	QueryPerformanceFrequency(&iFrequency);
	QueryPerformanceCounter(&iCount);
	m_iStart = iCount.QuadPart;
	m_iFrequency = iFrequency.QuadPart;
}

void Timer::stop()
{
	LARGE_INTEGER iCount;
	QueryPerformanceCounter(&iCount);
	m_iEnd = iCount.QuadPart;

	__int64 iIntervalTime = m_iEnd - m_iStart;
	m_dTimeMilliSeconds = double((iIntervalTime * 1000) / m_iFrequency);
}
