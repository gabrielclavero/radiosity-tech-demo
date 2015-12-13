//------------------------------------------------------------------------------------------
// File: Profiler.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "Profiler.h"

namespace DTFramework
{

Profiler::Profiler(const D3DDevicesManager &d3d)
:m_timer(d3d)
{
	
}

HRESULT Profiler::Init(const std::wstring &settingsFile)
{
	HRESULT hr = S_OK;

	m_shaders.clear();

	m_timer.Start();

	std::fstream settings;
	settings.exceptions(std::fstream::failbit | std::fstream::badbit);

	try 
	{
		settings.open(settingsFile);	

		std::string buffer;

		while(!settings.eof()) {
			while(isspace(settings.peek()) && !settings.eof()) settings.ignore();
			if(settings.eof()) break;

			settings >> buffer;

			if(buffer == "BEGINSHADER") 
				m_shaders.push_back(ShaderEntry());
			else if(buffer == "bandwidth") {
				if(m_shaders.size() == 0) { throw PROF_FILE_ERROR; }
				settings >> m_shaders[m_shaders.size()-1].bandwidthUsedPerGroup;
			} else if(buffer == "gflops") {
				if(m_shaders.size() == 0) { throw PROF_FILE_ERROR; }
				settings >> m_shaders[m_shaders.size()-1].gflopsUsedPerGroup;
			}
		}
	} 
	catch (ProfFileError &) 
	{
		MiscErrorWarning(PROFFILE_ERROR);
		hr = E_FAIL;
	}
	catch (std::fstream::failure &) 
	{
		MiscErrorWarning(IFSTREAM_ERROR);
		hr = E_FAIL;
	}
	catch (std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		hr = E_FAIL;
	}

	if(settings.is_open())
		settings.close();

	return hr;
}

void Profiler::StartEvent(const UINT shaderIndex, const UINT numGroups)
{
	_ASSERT(shaderIndex < m_shaders.size());

	if(shaderIndex >= m_shaders.size()) {
		MiscErrorWarning(INVALID_PARAMETER, L"Profiler::StartEvent");
		return;
	}

	m_currentShader = shaderIndex;
	m_currentNumGroups = numGroups;

	m_timer.UpdateForGPU();
}

void Profiler::EndEvent()
{
	_ASSERT(m_currentShader < m_shaders.size());

	if(m_currentShader >= m_shaders.size()) {
		MiscErrorWarning(INVALID_PARAMETER, L"Profiler::EndEvent");
		return;
	}

	m_timer.UpdateForGPU();

	const double timeElapsed = m_timer.GetTimeElapsed();

	if(timeElapsed <= 0) return;

	m_shaders[m_currentShader].numShaderExecutions += 1;

	//En nuestros shaders todos los grupos realizan la misma cantidad de operaciones de punto flotante y leen/escriben
	//la misma cantidad de datos, por lo tanto esta simple división sigue siendo la manera correcta de calcular estas métricas.
	//La fórmula es la utilizada en http://docs.nvidia.com/cuda/cuda-c-best-practices-guide/index.html#effective-bandwidth-calculation
	//pero se toma 1GB = 2^30 bytes.
	const double bandwidthAchieved = (m_shaders[m_currentShader].bandwidthUsedPerGroup * m_currentNumGroups) / timeElapsed;

	const double gflopsAchieved = (m_shaders[m_currentShader].gflopsUsedPerGroup * m_currentNumGroups) / timeElapsed;

	if(m_shaders[m_currentShader].numShaderExecutions == 1) {
		m_shaders[m_currentShader].maxBandwidth = bandwidthAchieved;
		m_shaders[m_currentShader].minBandwidth = bandwidthAchieved;

		m_shaders[m_currentShader].maxGflops = gflopsAchieved;
		m_shaders[m_currentShader].minGflops = gflopsAchieved;
	} else {
		m_shaders[m_currentShader].maxBandwidth = max(bandwidthAchieved, m_shaders[m_currentShader].maxBandwidth);
		m_shaders[m_currentShader].minBandwidth = min(bandwidthAchieved, m_shaders[m_currentShader].minBandwidth);

		m_shaders[m_currentShader].maxGflops = max(gflopsAchieved, m_shaders[m_currentShader].maxGflops);
		m_shaders[m_currentShader].minGflops = min(gflopsAchieved, m_shaders[m_currentShader].minGflops);
	}

	m_shaders[m_currentShader].bandwidthSum += bandwidthAchieved;
	m_shaders[m_currentShader].gflopsSum += gflopsAchieved;
	m_shaders[m_currentShader].totalTimeElapsed += timeElapsed;
}

void Profiler::EndProfiling(std::ofstream &outputFile)
{
	for(UINT i=0; i<m_shaders.size(); ++i) {
		if(m_shaders[i].numShaderExecutions == 0) continue;

		outputFile 
			<< std::endl << "SHADER " << i << std::endl
			<< "Number of executions: " << m_shaders[i].numShaderExecutions << std::endl
			<< "Avg. Memory Bandwidth: " << m_shaders[i].bandwidthSum / m_shaders[i].numShaderExecutions << "GB/S" << std::endl
			<< "Max. Memory Bandwidth: " << m_shaders[i].maxBandwidth << std::endl
			<< "Min. Memory Bandwidth: " << m_shaders[i].minBandwidth << std::endl
			<< "Avg. GFLOPs: " << m_shaders[i].gflopsSum / m_shaders[i].numShaderExecutions << std::endl
			<< "Max. GFLOPs: " << m_shaders[i].maxGflops << std::endl
			<< "Min. GFLOPs: " << m_shaders[i].minGflops << std::endl
			<< "Total Time: " << m_shaders[i].totalTimeElapsed << " seconds." << std::endl;
	}

	outputFile << std::endl;
}

void Profiler::ResetCounters()
{
	for(UINT i=0; i<m_shaders.size(); ++i) {
		m_shaders[i].bandwidthSum = 0;
		m_shaders[i].gflopsSum = 0;
		m_shaders[i].maxBandwidth = 0;
		m_shaders[i].minBandwidth = 0;
		m_shaders[i].maxGflops = 0;
		m_shaders[i].minGflops = 0;
		m_shaders[i].numShaderExecutions = 0;
		m_shaders[i].totalTimeElapsed = 0;
	}
}

}