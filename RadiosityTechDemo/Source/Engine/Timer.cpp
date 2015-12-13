//------------------------------------------------------------------------------------------
// File: Timer.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Timer.h"

namespace DTFramework
{

Timer::Timer(const D3DDevicesManager &d3d)
:m_d3dManager(d3d), m_prevTime(0), m_currentTime(0), m_countsPerSecond(0), m_secondsPerCount(0), m_framesPerSecond(0), m_framesTmp(0)
{
	
}

void Timer::Start()
{
	if(QueryPerformanceFrequency((LARGE_INTEGER *) &m_countsPerSecond) == 0) 
	{
		ErrorWarning(L"Timer::Start --> QueryPerformanceFrequency");
		return;
	}

	m_secondsPerCount = 1.0/  static_cast<double>(m_countsPerSecond);

	if(QueryPerformanceCounter((LARGE_INTEGER *) &m_prevTime) == 0) 
	{
		ErrorWarning(L"Timer::Start --> QueryPerformanceCounter");
		return;
	}

	m_currentTime = m_prevTime;
}

void Timer::Update()
{
	m_prevTime = m_currentTime;

	if(QueryPerformanceCounter((LARGE_INTEGER *) &m_currentTime) == 0) 
	{
		ErrorWarning(L"Timer::Update --> QueryPerformanceCounter");
		return;
	}

	static __int64 second = m_currentTime;

	if(m_currentTime - second < m_countsPerSecond)
		m_framesTmp++;
	else 
	{
		second = m_currentTime;
		m_framesPerSecond = m_framesTmp;
		m_framesTmp = 0;
	}
}

//un update que primero espera a que finalicen todas las tareas en la cola de tareas de la GPU
void Timer::UpdateForGPU()
{
	WaitGPUtoFinish();
	Update();
}

double Timer::GetTimeElapsed() const 
{
	return static_cast<double>(m_secondsPerCount*(m_currentTime - m_prevTime));
}

UINT Timer::GetFramesPerSecond() const
{
	return m_framesPerSecond;
}

//Utilizado para el profiling. Ejecutamos todas las sentencias en la cola de tareas de la GPU para saber que sólo mediremos 
//el tiempo de ejecución de la siguiente tarea que le daremos.
//Esto es necesario pues las tareas que le enviamos a la GPU ésta las ejecuta de manera asíncrona.
HRESULT Timer::WaitGPUtoFinish() const
{
	HRESULT hr;

	D3D11_QUERY_DESC queryDesc;
	queryDesc.Query = D3D11_QUERY_EVENT;
	queryDesc.MiscFlags = 0;
	ID3D11Query *eventQuery;
	if(FAILED(hr = m_d3dManager.CreateQuery( &queryDesc, &eventQuery ))) return hr;

	m_d3dManager.End(eventQuery); //insertar una valla en el pushbuffer
	while( m_d3dManager.GetData( eventQuery, NULL, 0, 0 ) != S_OK ) {}  //esperar hasta que el evento termine

	eventQuery->Release();

	return S_OK;
}


}