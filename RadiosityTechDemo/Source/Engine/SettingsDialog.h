//------------------------------------------------------------------------------------------
// File: SettingsDialog.h
//
// En esta clase se muestra un cuadro de diálogo que permite escoger parámetros de creación
// del ID3D11Device y del cálculo de iluminación global. Además permite elegir la escena 
// que se cargará.
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <sstream>

#include "resource.h"
#include "Utility.h"

#include <windowsx.h>

using std::vector;
using std::wstring;

namespace DTFramework
{

class SettingsDialog
{
public:
	SettingsDialog();
	~SettingsDialog();

	//muestra el cuadro de diálogo y carga su información relevante
	INT_PTR ShowConfigurationDialog(const DXGI_FORMAT, const wstring &, const UINT, const UINT );

	//procesa los comandos enviados al cuadro de diálogo y los guarda en el objeto
	INT_PTR InitialSettings(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam);

	const bool IsWindowed() const;
	const bool IsVsync() const;
	const bool IsAAEnabled() const;
	const bool IsGIEnabled() const;
	const bool IsCpuGIEnabled() const;
	const bool IsExportHemicubesEnabled() const;
	const bool IsProfilingEnabled() const;
	const UINT GetVerticesBakedPerDispatch() const;
	const UINT GetVerticesBakedPerDispatch2() const;
	const UINT GetNumBounces() const;

	DXGI_MODE_DESC const &GetSelectedDisplayMode() const;

	const wstring &GetSceneFileName() const;

private:
	static const int MINIMUM_RR = 60;

	DXGI_MODE_DESC m_selectedDisplayMode;       // display mode elegido por el usuario

	DXGI_MODE_DESC *m_displayModes;
	
	DXGI_ADAPTER_DESC m_primaryAdapterDesc;

	vector<UINT> m_realIndex;                  //algunos DXGI_MODE_DESC no son listados en el cuadro de diálogo. Aquí devolvemos el índice real 
	                                          //para m_displayModes del iésimo elemento de la lista
	wstring m_sceneFile;

	DXGI_FORMAT m_pixelFormat;
	wstring m_curDir;
	UINT m_minWidth;
	UINT m_minHeight;

	UINT m_nModes;
	bool m_windowed;                    // corremos la aplicacion en modo ventana?
	bool m_vsync;                       // v sync enabled?
	bool m_aa;                          // antialiasing enabled?
	bool m_gi;                          // gi enabled?
	bool m_cpuGi;                       // usar la cpu para calcular la GI?
	bool m_exportHemicubes;             // exportar los hemicubos a BMP?
	bool m_profiling;                   // activar profiling?
	UINT m_verticesBakedPerDispatch;    // cuántos vértices cocinaremos en cada 1er dispatch del algoritmo GPU
	UINT m_verticesBakedPerDispatch2;   // cuántos dispatches del primer paso de integración realizaremos antes de lanzar el segundo paso
	UINT m_numBounces;                  // número de rebotes de la luz que el algoritmo de radiosidad computará
};

inline const bool SettingsDialog::IsWindowed() const
{
	return m_windowed;
}

inline const bool SettingsDialog::IsVsync() const 
{
	return m_vsync;
}

inline const bool SettingsDialog::IsAAEnabled() const
{
	return m_aa;
}

inline const bool SettingsDialog::IsGIEnabled() const
{
	return m_gi;
}

inline const bool SettingsDialog::IsCpuGIEnabled() const
{
	return m_cpuGi;
}

inline const bool SettingsDialog::IsExportHemicubesEnabled() const
{
	return m_exportHemicubes;
}

inline const bool SettingsDialog::IsProfilingEnabled() const
{
	return m_profiling;
}

inline const UINT SettingsDialog::GetVerticesBakedPerDispatch() const
{
	return m_verticesBakedPerDispatch;
}

inline const UINT SettingsDialog::GetVerticesBakedPerDispatch2() const
{
	return m_verticesBakedPerDispatch2;
}

inline const UINT SettingsDialog::GetNumBounces() const
{
	return m_numBounces;
}

inline DXGI_MODE_DESC const &SettingsDialog::GetSelectedDisplayMode() const
{
	return m_selectedDisplayMode;
}

inline const wstring &SettingsDialog::GetSceneFileName() const
{
	return m_sceneFile;
}

}

#endif
