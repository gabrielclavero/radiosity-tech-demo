//------------------------------------------------------------------------------------------
// File: Timer.h
//
// Esta clase permite calcular el tiempo transcurrido entre eventos determinados con una
// precisión del orden de los microsegundos.
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef TIMER_H
#define TIMER_H

#include "Utility.h"
#include "D3DDevicesManager.h"

namespace DTFramework
{

class Timer
{
public:
	Timer(const D3DDevicesManager &d3d);

	void Start();
	void Update();
	void UpdateForGPU();                //no obtiene el timestamp hasta que todas las tareas en la cola de la GPU hayan terminado


	double GetTimeElapsed() const;      //en segundos
	UINT GetFramesPerSecond() const;

private:
	HRESULT WaitGPUtoFinish() const;

private:
	const D3DDevicesManager &m_d3dManager;

	__int64 m_prevTime;
	__int64 m_currentTime;

	__int64 m_countsPerSecond;
	double m_secondsPerCount;

	UINT m_framesPerSecond;
	UINT m_framesTmp;
};

}

#endif