//------------------------------------------------------------------------------------------
// File: D3DDevicesManager.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "D3DDevicesManager.h"

namespace DTFramework
{

D3DDevicesManager::D3DDevicesManager()
: m_device11(0), m_deviceContext(0), m_swapChain(0), m_device10(0), m_renderTargetView(0), m_depthStencilView(0),
 m_vsync(0), m_MSsampleCount(1), m_MSqualityLevel(0), m_font(0), m_ready(false)
{

}

D3DDevicesManager::~D3DDevicesManager()
{
	SAFE_RELEASE(m_font);

	SAFE_RELEASE(m_renderTargetView);
	SAFE_RELEASE(m_depthStencilView);

	SAFE_RELEASE(m_swapChain);
	SAFE_RELEASE(m_deviceContext);

	//descomentar para testear memory leaks de interfaces COM
	//no olvidar setear los flags de creación del device con D3D11_CREATE_DEVICE_DEBUG
	/*ID3D11Debug *debugDevice = nullptr;
	HRESULT result = m_device11->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void **>(&debugDevice));

	result = debugDevice->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
	SAFE_RELEASE(debugDevice);*/

	SAFE_RELEASE(m_device11);
	SAFE_RELEASE(m_device10);
}

HRESULT D3DDevicesManager::Init(const DXGI_MODE_DESC &mode, const bool windowed, const bool vsync, const bool aa, 
                                const HWND window, const UINT totalBackBuffers)
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"D3DDevicesManager::Init");
		return E_FAIL;
	}

	HRESULT hr;

	//vsync
	m_vsync = vsync;

	//d3d devices y swapchain
	if(FAILED( hr = PrepareDevicesAndSwapChain(mode, windowed, aa, window, totalBackBuffers) ))
		return hr;
	
	//obtener un render target view del backbuffer
	ID3D11Texture2D *backBuffer = NULL;
	if(FAILED(hr = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID *)&backBuffer))) 
	{
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::Init-->GetBuffer");
		return hr;
	}
	if(FAILED(hr = m_device11->CreateRenderTargetView(backBuffer, NULL, &m_renderTargetView))) 
	{
		SAFE_RELEASE(backBuffer);
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::Init-->CreateRenderTargetView");
		return hr;
	}

	//la llamada a GetBuffer incrementa en 1 las referencias al back buffer.
	SAFE_RELEASE(backBuffer);

	//depth stencil buffer
	if(FAILED( hr = PrepareDepthStencilBuffer() )) return hr;
	
	//bindear las views al Output Merger Stage del pipeline
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	//view port
	if(FAILED( hr = PrepareViewPort() )) return hr;

	//font
	IFW1Factory *FW1factory;
	if(FAILED(hr = FW1CreateFactory(FW1_VERSION, &FW1factory))) return hr;
	if(FAILED(hr = FW1factory->CreateFontWrapper(m_device11, L"Arial", &m_font))) {
		SAFE_RELEASE(FW1factory);
		return hr;
	}
	SAFE_RELEASE(FW1factory);

	m_ready = true;

	return hr;
}

//creamos el ID3D10Device, ID3D11Device, ID3D11DeviceContext, y IDXGISwapChain. Utilizamos las opciones que escogió el usuario en el SettingsDialog y en el EngineConfig
HRESULT D3DDevicesManager::PrepareDevicesAndSwapChain(const DXGI_MODE_DESC &mode, const bool windowed, const bool aa, const HWND window, const UINT totalBackBuffers)
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC sd;

	//copiamos los parametros elegidos en el SettingsDialog
	if(memcpy_s(&(sd.BufferDesc), sizeof(DXGI_MODE_DESC), &mode, sizeof(DXGI_MODE_DESC)) != 0) {
		MiscErrorWarning(MEMCPY);
		return E_FAIL;
	}

	//no multisampling
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = totalBackBuffers;
	sd.OutputWindow = window;
	sd.Windowed = windowed;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//refresh rate. vsync activado => 60 cuadros por segundo. Desactivado => se dibujará tantas veces por segundo como se pueda
	if(m_vsync) {
		sd.BufferDesc.RefreshRate.Numerator = 0;
		sd.BufferDesc.RefreshRate.Denominator = 1;
	}

	//feature level
	D3D_FEATURE_LEVEL featureLevel[1] = {D3D_FEATURE_LEVEL_11_0};
	
	//flags
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;	// | D3D11_CREATE_DEVICE_DEBUG;
	#if defined(DEBUG)
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif

	// multisampling o antialiasing. 
	if(aa)
	{
		//creamos un device temporal para ver si el antialiasing requerido está disponible
		ID3D11Device *device;
		ID3D11DeviceContext *context;

		hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevel, 1, D3D11_SDK_VERSION, &device, NULL, &context);

		if(FAILED(hr)) {
			DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::PrepareDevicesAndSwapChain-->D3D11CreateDevice");
			return hr;
		}

		UINT qualityLevels;

		//chequeamos que esté disponible en la pc actual y recordar que el depthbuffer debe tener los mismos valores
		if(FAILED( hr = device->CheckMultisampleQualityLevels(sd.BufferDesc.Format, SAMPLE_COUNT, &qualityLevels) ) ) {
			DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::PrepareDevicesAndSwapChain-->ID3D11Device::CheckMultisampleQualityLevels");
			device->Release();
			context->Release();
			return hr;
		}

		//si devolvió al menos un quality level usamos el SAMPLE_COUNT, sino 1. Ver DXGI_SAMPLE_DESC en la documentación de directx
		sd.SampleDesc.Count = qualityLevels == 0 ? 1 : SAMPLE_COUNT;
		sd.SampleDesc.Quality = 0;

		//guardar estos valores para que el depth buffer utilice los mismos que el back buffer
		m_MSsampleCount = sd.SampleDesc.Count;
		m_MSqualityLevel = sd.SampleDesc.Quality;

		device->Release();
		context->Release();
	}

	//crear el swap chain, device y device context de direct3d11.
	hr = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevel, 1, 
									   D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device11, NULL, &m_deviceContext);

	if(FAILED(hr)) {
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::PrepareDevicesAndSwapChain-->D3D11CreateDeviceAndSwapChain");
		return hr;
	}

	//en algunas partes también necesitaremos un ID3D10Device. Aquí lo creamos.
	createDeviceFlags = D3D10_CREATE_DEVICE_SINGLETHREADED;
	#if defined(DEBUG)
		createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	#endif
	hr = D3D10CreateDevice(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, D3D10_SDK_VERSION, &m_device10);

	if(FAILED(hr))
		DXGI_D3D_ErrorWarning(hr, L"D3DDevicesManager::PrepareDevicesAndSwapChain-->D3D10CreateDevice");

	return hr;
}

HRESULT D3DDevicesManager::PrepareDepthStencilBuffer()
{
	//crear el depth stencil buffer usando un texture resource
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC sd;

	if(FAILED( hr = m_swapChain->GetDesc(&sd) )) {
		DXGI_D3D_ErrorWarning(hr, L"PrepareDepthStencilBufferAndView --> IDXGISwapChain::GetDesc");
		return hr;
	}

	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;                    //para el depth buffer con un mip map level nos basta
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;   //no stencil buffer
	descDepth.SampleDesc.Count = m_MSsampleCount;
	descDepth.SampleDesc.Quality = m_MSqualityLevel;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;

	ID3D11Texture2D *depthStencilBuffer;
	if(FAILED(hr = m_device11->CreateTexture2D( &descDepth, NULL, &depthStencilBuffer ))) {
		DXGI_D3D_ErrorWarning(hr, L"PrepareDepthStencilBufferAndView --> ID3D11Device::CreateTexture2D");
		return hr;
	}

	//crear la view del depthstencil texture
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = descDepth.SampleDesc.Count > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = 0;
	if( FAILED(hr = m_device11->CreateDepthStencilView(depthStencilBuffer, &dsvDesc, &m_depthStencilView)) )
		DXGI_D3D_ErrorWarning(hr, L"PrepareDepthStencilBufferAndView --> ID3D11Device::CreateDepthStencilView");

	depthStencilBuffer->Release();	//la view guarda una referencia a la textura, asi que podemos quitar esta

	return hr;
}

HRESULT D3DDevicesManager::PrepareViewPort()
{
	HRESULT hr;

	DXGI_SWAP_CHAIN_DESC sd;
	if(FAILED( hr = m_swapChain->GetDesc(&sd) )) {
		DXGI_D3D_ErrorWarning(hr, L"PrepareViewPort --> IDXGISwapChain::GetDesc");
		return hr;
	}

	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;
	m_vp.Width = static_cast<float>(sd.BufferDesc.Width);
	m_vp.Height = static_cast<float>(sd.BufferDesc.Height);
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;
	m_deviceContext->RSSetViewports(1, &m_vp);

	return hr;
}

}