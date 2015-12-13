//------------------------------------------------------------------------------------------
// File: Profiler.h
//
// Simple profiler para medir la performance de los compute shaders del algoritmo de radiosidad.
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef PROFILER_H
#define PROFILER_H

#include <algorithm>

#include "Utility.h"
#include "D3DDevicesManager.h"
#include "Timer.h"

using std::vector;
using std::max;
using std::min;

namespace DTFramework
{

class Profiler
{
public:
	Profiler(const D3DDevicesManager &d3d);

	HRESULT Init(const std::wstring &settingsFile);

	void StartEvent(const UINT shaderIndex, const UINT numGroups);
	void EndEvent();

	void EndProfiling(std::ofstream &outputFile);

	void ResetCounters();

private:
	struct ShaderEntry 
	{
		double bandwidthUsedPerGroup;       //en gigabytes
		double gflopsUsedPerGroup;         //en gigaflops

		double bandwidthSum;              //suma de los anchos de banda alcanzados en las ejecuciones de este shader
		double gflopsSum;
		UINT numShaderExecutions;        //cantidad de veces que hemos ejecutado este shader
		double maxBandwidth;
		double minBandwidth;
		double maxGflops;
		double minGflops;

		double totalTimeElapsed;        //suma total del tiempo de las ejecuciones de este shader

		ShaderEntry() 
		: bandwidthUsedPerGroup(0), gflopsUsedPerGroup(0), bandwidthSum(0), gflopsSum(0), numShaderExecutions(0), 
		  maxBandwidth(0), minBandwidth(0), maxGflops(0), minGflops(0), totalTimeElapsed(0)
		{

		}
	};

	enum ProfFileError { PROF_FILE_ERROR };

	Timer m_timer;

	UINT m_currentShader;
	UINT m_currentNumGroups;
	vector<ShaderEntry> m_shaders;
};

}

#endif