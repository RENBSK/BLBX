#include "ImGuiTimer.h"

using namespace ImGuiControl;

#if defined(_WIN32)

double ImGuiTimer::s_frequency = 0.0f;

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

ImGuiTimer::ImGuiTimer()
{
	if (s_frequency == 0.0f)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&s_frequency);
	}

    QueryPerformanceCounter((LARGE_INTEGER*)&m_start);
}

void ImGuiTimer::Reset()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&m_start);
}

double ImGuiTimer::GetTime() const
{
    double counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
    counter = (counter - m_start) / s_frequency;
    return counter;
}

float ImGuiTimer::GetMilliseconds() const
{
    double counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&counter);
	float ms = (float)(1000.0f * (counter - m_start) / s_frequency);
    return ms;
}

#elif defined(__linux__) || defined (__APPLE__)

#include <sys/time.h>

ImGuiTimer::ImGuiTimer()
{
    Reset();
}

void ImGuiTimer::Reset()
{
    timeval t;
    gettimeofday(&t, 0);
    m_start_sec = t.tv_sec;
    m_start_usec = t.tv_usec;
}

float ImGuiTimer::GetMilliseconds() const
{
    timeval t;
    gettimeofday(&t, 0);
    return 1000.0f * (t.tv_sec - m_start_sec) + 0.001f * (t.tv_usec - m_start_usec);
}

#else

ImGuiTimer::ImGuiTimer()
{
}

void ImGuiTimer::Reset()
{
}

float ImGuiTimer::GetMilliseconds() const
{
	return 0.0f;
}

#endif
