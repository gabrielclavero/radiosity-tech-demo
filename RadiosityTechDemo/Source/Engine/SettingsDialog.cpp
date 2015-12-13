//------------------------------------------------------------------------------------------
// File: SettingsDialog.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#include "SettingsDialog.h"

namespace DTFramework
{

static SettingsDialog *g_settingsDialog = NULL;


//función director callback para el manejador de mensajes del cuadro de dialogo inicial
BOOL CALLBACK InitialSettingsCB( HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	return (BOOL) g_settingsDialog->InitialSettings( hDlg, uiMsg, wParam, lParam );
}

SettingsDialog::SettingsDialog()
: m_displayModes(NULL),
m_pixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM), m_minWidth(0), m_minHeight(0), m_nModes(0), 
m_windowed(0), m_vsync(0), m_aa(false), m_gi(true), m_cpuGi(false), m_exportHemicubes(false), m_profiling(false),
m_verticesBakedPerDispatch(256), m_verticesBakedPerDispatch2(1), m_numBounces(2)
{
	
}

SettingsDialog::~SettingsDialog()
{
	SAFE_DELETE_ARRAY(m_displayModes);
}


INT_PTR SettingsDialog::ShowConfigurationDialog(const DXGI_FORMAT pixelFormat, const wstring &curDir, const UINT minWidth, const UINT minHeight)
{
	HRESULT hr;

	m_pixelFormat = pixelFormat;
	m_curDir = curDir;
	m_minWidth = minWidth;
	m_minHeight = minHeight;

	IDXGIFactory *factory = NULL;

	if(FAILED(hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &factory))) {
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->CreateDXGIFactory"); 
		return 0; 
	}
	
	//obtener el display adapter primario
	IDXGIAdapter *pAdapterTmp = NULL;
	if(FAILED(hr = factory->EnumAdapters(0, &pAdapterTmp))) {
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->EnumAdapters"); 
		SAFE_RELEASE(factory);
		return 0; 
	}

	//obtener la descripción del adapter
	if(FAILED(hr = pAdapterTmp->GetDesc(&m_primaryAdapterDesc))) {
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->GetDesc"); 
		SAFE_RELEASE(factory);
		SAFE_RELEASE(pAdapterTmp);
		return 0; 
	}

	//enumerar display modes del primer display adapter
	IDXGIOutput *pOutput = NULL; 
	
	//obtener la salida primaria
	if(FAILED( hr = pAdapterTmp->EnumOutputs(0,&pOutput) )) { 
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->EnumOutputs");
		SAFE_RELEASE(factory);
		SAFE_RELEASE(pAdapterTmp);
		return 0; 
	}

	//obtener el numero de elementos
	if(FAILED( hr = pOutput->GetDisplayModeList( m_pixelFormat, 0, &m_nModes, NULL) ) ) { 
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->GetDisplayModeList");
		SAFE_RELEASE(factory);
		SAFE_RELEASE(pAdapterTmp);
		SAFE_RELEASE(pOutput);
		return 0; 
	}

	if(m_nModes == 0) {
		MessageBox(NULL, L"No existen display modes para el formato DXGI elegido en su hardware actual.", L"Error", MB_OK);
		return 0;
	}

	if((m_displayModes = new (std::nothrow) DXGI_MODE_DESC[m_nModes]) == NULL) { 
		MiscErrorWarning(BAD_ALLOC); 
		SAFE_RELEASE(factory);
		SAFE_RELEASE(pAdapterTmp);
		SAFE_RELEASE(pOutput);
		return 0; 
	}

	m_realIndex.reserve(m_nModes);

	//obtener la lista
	if( FAILED(hr = pOutput->GetDisplayModeList(m_pixelFormat, 0, &m_nModes, m_displayModes)) ) { 
		DXGI_D3D_ErrorWarning(hr, L"SettingsDialog::ShowConfigurationDialog-->GetDisplayModeList"); 
		SAFE_RELEASE(factory);
		SAFE_RELEASE(pAdapterTmp);
		SAFE_RELEASE(pOutput);
		return 0; 
	}

	SAFE_RELEASE(factory);
	SAFE_RELEASE(pAdapterTmp);
	SAFE_RELEASE(pOutput);

	g_settingsDialog = this;

	return DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, InitialSettingsCB);
}

INT_PTR SettingsDialog::InitialSettings(HWND dialog, UINT msg, WPARAM wparam, LPARAM lparam)
{
	try {

		switch(msg)
		{
			case WM_INITDIALOG:
			{	
				//cargar nombre del display adapter primario
				SetWindowText(GetDlgItem(dialog, IDC_ADAPTERNAME), m_primaryAdapterDesc.Description);

				CheckDlgButton( dialog, IDC_WINDOWED, m_windowed = true );
				CheckDlgButton( dialog, IDC_FULLSCREEN, !m_windowed );

				//selector de escena
				wstring searchDir = m_curDir + SEARCH_SCENE_DIRECTORY;
				if(!DlgDirList(dialog, (PTSTR) searchDir.c_str(), IDC_LISTScene, 0, 0)) { 
					ErrorWarning(L"SettingsDialog::InitialSettings-->DlgDirList"); 
					EndDialog( dialog, IDC_CANCEL ); 
					break; 
				}

				//cargar valores en los siguientes combobox
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"2");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"4");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"8");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"16");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"32");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"64");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"128");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"256");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"512");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"1024");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED), L"2048");

				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x1");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x2");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x5");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x10");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x15");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x20");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x25");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x30");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x35");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x40");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x45");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x50");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x55");
				ComboBox_AddString(GetDlgItem(dialog, IDC_VERTICESBAKED3), L"x60");

				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"1");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"2");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"3");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"4");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"5");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"6");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"7");
				ComboBox_AddString(GetDlgItem(dialog, IDC_BOUNCES), L"8");

				//seleccionar valores por defecto
				ComboBox_SetCurSel(GetDlgItem(dialog, IDC_VERTICESBAKED), 9);
				ComboBox_SetCurSel(GetDlgItem(dialog, IDC_VERTICESBAKED3), 0);
				ComboBox_SetCurSel(GetDlgItem(dialog, IDC_BOUNCES), 1);

				CheckDlgButton( dialog, IDC_GICHECK, BST_CHECKED);
				CheckDlgButton( dialog, IDC_VSYNC, BST_CHECKED);
				break;
			}
			case WM_COMMAND:
			{
				switch( LOWORD(wparam) )
				{
					case IDC_GICHECK:	//click en gi enabled

						//desactivar o activar las opciones GI
						EnableWindow( GetDlgItem( dialog, IDC_CPUGICHECK ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );
						EnableWindow( GetDlgItem( dialog, IDC_EXPORTHEMICUBES ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );
						EnableWindow( GetDlgItem( dialog, IDC_PROFILING ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );
						EnableWindow( GetDlgItem( dialog, IDC_VERTICESBAKED ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );
						EnableWindow( GetDlgItem( dialog, IDC_BOUNCES ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );

						if( (Button_GetCheck(GetDlgItem(dialog, IDC_CPUGICHECK)) == BST_UNCHECKED) )
							EnableWindow( GetDlgItem( dialog, IDC_VERTICESBAKED3 ), (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED) );
						else
							EnableWindow( GetDlgItem( dialog, IDC_VERTICESBAKED3 ), false );

						break;

					case IDC_CPUGICHECK:	//click en cpu gi
						//desactivar o activar vertices baked per dispatch 2 (porque para CPU GI no se usa)
						EnableWindow( GetDlgItem( dialog, IDC_VERTICESBAKED3 ), (Button_GetCheck(GetDlgItem(dialog, IDC_CPUGICHECK)) == BST_UNCHECKED) );
						break;

					case IDC_WINDOWED:		//clic en windowed
						CheckDlgButton( dialog, IDC_FULLSCREEN, BST_UNCHECKED);
						CheckDlgButton( dialog, IDC_WINDOWED, BST_CHECKED);

						ComboBox_ResetContent( GetDlgItem( dialog, IDC_RESOLUTION ) );
						EnableWindow( GetDlgItem( dialog, IDC_RESOLUTION ), false );

						break;

					case IDC_FULLSCREEN:	//clic en full screen
						CheckDlgButton( dialog, IDC_FULLSCREEN, BST_CHECKED);
						CheckDlgButton( dialog, IDC_WINDOWED, BST_UNCHECKED);

						EnableWindow( GetDlgItem( dialog, IDC_RESOLUTION ), true );

						m_realIndex.clear();

						//cargando resoluciones y tasas de refresco (solo las que son mas altas de 60)
						for(UINT i=0; i<m_nModes; ++i) 
						{
							if(m_displayModes[i].RefreshRate.Denominator == 0) continue;

							WCHAR tmp[150];
							int refreshRate =  static_cast<int> (ceil(static_cast<float> ( m_displayModes[i].RefreshRate.Numerator ) / 
							                                          static_cast<float> ( m_displayModes[i].RefreshRate.Denominator) ) );

							swprintf(tmp, 150, L"%ix%i - %i Hz - Formato: %i", m_displayModes[i].Width, m_displayModes[i].Height, refreshRate, m_displayModes[i].Format);

							if(refreshRate >= MINIMUM_RR && (m_displayModes[i].Width >= m_minWidth || m_displayModes[i].Height >= m_minHeight )) {
								int res = ComboBox_AddString(GetDlgItem(dialog, IDC_RESOLUTION), tmp);
								m_realIndex.push_back(i);
								if(res == CB_ERR || res == CB_ERRSPACE) {
									ErrorWarning(L"SettingsDialog::InitialSettings-->ComboBox_AddString"); 
									EndDialog( dialog, IDC_CANCEL ); 
									break; 
								}					
							}
						}
						ComboBox_SetCurSel(GetDlgItem(dialog, IDC_RESOLUTION), 0);

						break;
			
					case IDC_CANCEL:
						EndDialog( dialog, IDC_CANCEL );
						break;

					case IDC_OK:     //si hacemos click en Aceptar guardamos todas las settings elegidas en las variables miembro
					case IDOK:      //habilitamos la tecla enter para Aceptar también 
						UINT ok = IDC_OK;

						//windowed, vsync, aa
						m_windowed = (Button_GetCheck(GetDlgItem(dialog, IDC_WINDOWED)) == BST_CHECKED);
						m_vsync = (Button_GetCheck(GetDlgItem(dialog, IDC_VSYNC)) == BST_CHECKED);
					
						m_aa = (Button_GetCheck(GetDlgItem(dialog, IDC_AACHECK)) == BST_CHECKED);

						//GI Settings
						m_gi = (Button_GetCheck(GetDlgItem(dialog, IDC_GICHECK)) == BST_CHECKED);
						m_cpuGi = (Button_GetCheck(GetDlgItem(dialog, IDC_CPUGICHECK)) == BST_CHECKED);
						m_exportHemicubes = (Button_GetCheck(GetDlgItem(dialog, IDC_EXPORTHEMICUBES)) == BST_CHECKED);
						m_profiling = (Button_GetCheck(GetDlgItem(dialog, IDC_PROFILING)) == BST_CHECKED);

						WCHAR GIValuesTMP[50];
						std::wstringstream wss;
						wss.exceptions( std::wstringstream::failbit | std::wstringstream::badbit );

						SendMessage(GetDlgItem(dialog, IDC_VERTICESBAKED), CB_GETLBTEXT, ComboBox_GetCurSel(GetDlgItem(dialog, IDC_VERTICESBAKED)), (LPARAM) GIValuesTMP);
						wss << GIValuesTMP;
						wss >> m_verticesBakedPerDispatch;

						SendMessage(GetDlgItem(dialog, IDC_VERTICESBAKED3), CB_GETLBTEXT, ComboBox_GetCurSel(GetDlgItem(dialog, IDC_VERTICESBAKED3)), (LPARAM) GIValuesTMP);
						wss.clear(std::wstringstream::goodbit);
						wss << GIValuesTMP;
						wss.ignore();
						wss >> m_verticesBakedPerDispatch2;

						SendMessage(GetDlgItem(dialog, IDC_BOUNCES), CB_GETLBTEXT, ComboBox_GetCurSel(GetDlgItem(dialog, IDC_BOUNCES)), (LPARAM) GIValuesTMP);
						wss.clear(std::wstringstream::goodbit);
						wss << GIValuesTMP;
						wss >> m_numBounces;


						//guardar el nombre de la escena
						WCHAR sceneName[MAX_PATH];
						//obtenemos el indice de la escena elegida en la list box
						UINT indexScene = static_cast<UINT> ( SendMessage(GetDlgItem(dialog, IDC_LISTScene), LB_GETCURSEL, 0, 0) );	
						if(indexScene == LB_ERR) {
							MessageBox(NULL, L"Elija una escena para cargar.", L"Error", MB_OK);
							break;
						}
						//si la longitud excede MAX_PATH -1 caracteres mostramos mensaje
						if( SendMessage(GetDlgItem(dialog, IDC_LISTScene), LB_GETTEXTLEN, indexScene, 0) > MAX_PATH-1) { 
							MessageBox(NULL, L"Nombre de archivo muy largo.", L"Error", MB_OK);
							break;
						}
						//copiamos nombre de scene elegida al buffer temporal
						if( SendMessage(GetDlgItem(dialog, IDC_LISTScene), LB_GETTEXT, indexScene, (LPARAM) sceneName) == LB_ERR) { 
							MessageBox(NULL, L"Elija una escena para cargar.", L"Error", MB_OK);
							break;
						}
						m_sceneFile = wstring(sceneName);

										
						//si elegimos fullscreen copiamos el setting elegido sino elegimos el primero que tenga el ancho y alto requerido asi como el refresh rate
						if(!m_windowed) 
						{
							int index = ComboBox_GetCurSel(GetDlgItem(dialog, IDC_RESOLUTION));
							if(index == CB_ERR) {
								MessageBox(NULL, L"Resolución incorrecta.", L"Error", MB_OK);
								ok = IDC_CANCEL;
							}

							UINT realIndex = m_realIndex[index];
							if(realIndex >= m_nModes || realIndex < 0) {
								MessageBox(NULL, L"Error interno.", L"Error", MB_OK);
								ok = IDC_CANCEL;
							}

							if( memcpy_s(&m_selectedDisplayMode, sizeof(DXGI_MODE_DESC), &(m_displayModes[realIndex]), sizeof(DXGI_MODE_DESC)) != 0) {
								MiscErrorWarning( WCSCPYERROR ); 
								EndDialog( dialog, IDC_CANCEL ); 
								break; 
							}
						} 
						else 
						{
							UINT i=0;
							for(i=0; i<m_nModes; ++i) 
							{
								if(m_displayModes[i].RefreshRate.Denominator == 0) continue;

								int refreshRate =  static_cast<int> (ceil(static_cast<float> ( m_displayModes[i].RefreshRate.Numerator ) / 
								                                          static_cast<float> ( m_displayModes[i].RefreshRate.Denominator) ));
								if(m_displayModes[i].Width == m_minWidth && m_displayModes[i].Height == m_minHeight && refreshRate >= MINIMUM_RR)
									break;
							}
							if(i < m_nModes) {
								if( memcpy_s(&m_selectedDisplayMode, sizeof(DXGI_MODE_DESC), &(m_displayModes[i]), sizeof(DXGI_MODE_DESC)) != 0) {
									MiscErrorWarning( WCSCPYERROR ); 
									EndDialog( dialog, IDC_CANCEL ); 
									break; 
								}
							} 
							else {
								MessageBox(NULL, L"No hay modos DXGI disponibles con la configuración actual.", L"Error", MB_OK);
								ok = IDC_CANCEL;
							}
						}

						EndDialog(dialog, ok);
						break;
				}
			}
			case WM_QUIT:
				if(wparam == 2)
					EndDialog( dialog, IDC_CANCEL );
				break;
			default:
				return false;	//los mensajes no atendidos pasan al proceso por defecto (Ver DialogProc en MSDN)
		}
	}
	catch (std::wstringstream::failure &) 
	{
		MiscErrorWarning(SS_ERROR);
		EndDialog( dialog, IDC_CANCEL );
	}
	catch (std::bad_alloc &) 
	{
		MiscErrorWarning(BAD_ALLOC);
		EndDialog( dialog, IDC_CANCEL );
	}
	catch (std::length_error &) 
	{
		MiscErrorWarning(LENGTH_ERROR);
		EndDialog( dialog, IDC_CANCEL );
	}


	return true;	//atendimos el mensaje
}

}