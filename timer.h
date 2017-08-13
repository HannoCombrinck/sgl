#pragma once

#include <string>

class Timer
{
public:
	Timer(const std::string& sName);
	~Timer();
	
	std::string getName() const { return m_sName; }
	double getTime() { return m_dTimeMilliSeconds; }
	void start();
	void stop();
	
private:
	std::string m_sName;
	__int64 m_iStart;
	__int64 m_iEnd;	
	__int64 m_iFrequency;
	double m_dTimeMilliSeconds;
};
