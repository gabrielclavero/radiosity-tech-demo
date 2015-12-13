//------------------------------------------------------------------------------------------
// File: D3D11Resources.h
//
// Aquí definimos clases que funcionan como un wrapping para algunos resources de uso frecuente
// de d3d11.
// 
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------

#ifndef D3D11_RESOURCES
#define D3D11_RESOURCES

#include "Utility.h"
#include "D3DDevicesManager.h"

namespace DTFramework
{


class Buffer
{
public:
	Buffer(const D3DDevicesManager &d3d, const UINT byteWidth)
	: m_d3dManager(d3d), m_buffer(0), m_byteWidth(byteWidth), m_ready(false)
	{

	}
	
	virtual ~Buffer()
	{
		SAFE_RELEASE(m_buffer);
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init() = 0;

	ID3D11Buffer *GetBuffer() const
	{
		return m_buffer;
	}

	const UINT GetByteWidth() const
	{
		return m_byteWidth;
	}

protected:
	const D3DDevicesManager &m_d3dManager;

	ID3D11Buffer *m_buffer;

	//Es const. Para reforzar el hecho de que una vez que llamaste al método Init el Buffer ya ha sido creado en
	//GPU RAM y para que tenga otro bytewidth debe crearse de nuevo
	const UINT m_byteWidth;

	bool m_ready;
};

class WritableBuffer : public Buffer
{
public:
	WritableBuffer(const D3DDevicesManager &d3d, const UINT byteWidth, DXGI_FORMAT format, const UINT numElements)
	: Buffer(d3d, byteWidth), m_srv(0), m_uav(0), m_format(format), m_numElements(numElements)
	{

	}

	virtual ~WritableBuffer()
	{
		SAFE_RELEASE(m_srv);
		SAFE_RELEASE(m_uav);
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	ID3D11ShaderResourceView *GetShaderResourceView() const
	{
		return m_srv;
	}

	ID3D11UnorderedAccessView *GetUnorderedAccessView() const
	{
		return m_uav;
	}

	const UINT GetNumElements() const
	{
		return m_numElements;
	}

private:
	ID3D11ShaderResourceView *m_srv;
	ID3D11UnorderedAccessView *m_uav;

	const DXGI_FORMAT m_format;
	const UINT m_numElements;
};


class VertexBuffer : public Buffer
{
public:
	VertexBuffer(const D3DDevicesManager &d3d, const UINT byteWidth, const void * const initData = NULL)
	: Buffer(d3d, byteWidth), m_initData(initData)
	{

	}

	virtual ~VertexBuffer()
	{
		
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

private:
	const void * const m_initData;
};

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const D3DDevicesManager &d3d, const UINT byteWidth, const void * const initData = NULL)
	: Buffer(d3d, byteWidth), m_initData(initData)
	{

	}

	virtual ~IndexBuffer()
	{
		
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

private:
	const void * const m_initData;
};

class ConstantBuffer : public Buffer
{
public:
	ConstantBuffer(const D3DDevicesManager &d3d, const UINT byteWidth, const void * const initData = NULL, const bool isDynamic = true)
	: Buffer(d3d, byteWidth), m_initData(initData), m_dynamic(isDynamic)
	{

	}

	virtual ~ConstantBuffer()
	{
		
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();
	
	HRESULT Update(const void * const data, const SIZE_T size); 

private:
	const void * m_initData;
	const bool m_dynamic;
};

class StagingBuffer : public Buffer
{
public:
	StagingBuffer(const D3DDevicesManager &d3d, const UINT byteWidth)
	: Buffer(d3d, byteWidth)
	{

	}

	virtual ~StagingBuffer()
	{
		
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	const float *GetMappedData(ID3D11Buffer *buf) const;
	void CloseMappedData() const;
};

class ImmutableBuffer : public Buffer
{
public:
	ImmutableBuffer(const D3DDevicesManager &d3d, const UINT byteWidth, const UINT elementWidth, const void * const data=NULL, const DXGI_FORMAT format=DXGI_FORMAT_R32G32B32A32_FLOAT)
	: Buffer(d3d, byteWidth), m_srv(0), m_elementWidth(elementWidth), m_initData(data), m_format(format)
	{

	}

	virtual ~ImmutableBuffer()
	{
		SAFE_RELEASE(m_srv);
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	ID3D11ShaderResourceView *GetShaderResourceView() const
	{
		return m_srv;
	}

private:
	ID3D11ShaderResourceView *m_srv;
	const UINT m_elementWidth;
	const void * const m_initData;
	const DXGI_FORMAT m_format;
};

class Texture
{
public:
	Texture(const D3DDevicesManager &d3d, const UINT width, const UINT height, const DXGI_FORMAT format)
	: m_d3dManager(d3d), m_texture(0), m_width(width), m_height(height), m_format(format), m_ready(false)
	{

	}
	virtual ~Texture()
	{
		SAFE_RELEASE(m_texture);
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init() = 0;

	ID3D11Texture2D *GetTexture() const
	{
		return m_texture;
	}

protected:
	const D3DDevicesManager &m_d3dManager;

	ID3D11Texture2D *m_texture;
	const UINT m_width;
	const UINT m_height;
	const DXGI_FORMAT m_format;

	bool m_ready;
};

//ID3D11Texture2D sin anti aliasing y sus views asociadas
class Texture2D_NOAA : public Texture
{
public:
	Texture2D_NOAA(const D3DDevicesManager &d3d, const UINT width, const UINT height, const UINT mipLevels, const UINT arraySize, 
	               const DXGI_FORMAT format, const D3D11_USAGE usage, const UINT bindFlags, const UINT cpuFlags=0, const UINT miscFlags=0,
	               const D3D11_SUBRESOURCE_DATA * const initialData=NULL)
	: Texture(d3d, width, height, format), m_dsv(0), m_srv(0), m_rtv(0), m_mipLevels(mipLevels), m_arraySize(arraySize), m_usage(usage),
	m_bindFlags(bindFlags), m_cpuFlags(cpuFlags), m_miscFlags(miscFlags), m_initData(initialData)
	{
	
	}

	virtual ~Texture2D_NOAA()
	{
		SAFE_RELEASE(m_dsv);
		SAFE_RELEASE(m_srv);
		SAFE_RELEASE(m_rtv);
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	ID3D11RenderTargetView *GetRenderTargetView()
	{
		return m_rtv;
	}
	ID3D11ShaderResourceView *GetShaderResourceView()
	{
		return m_srv;
	}
	ID3D11DepthStencilView *GetDepthStencilView()
	{
		return m_dsv;
	}

private:
	ID3D11DepthStencilView *m_dsv;
	ID3D11ShaderResourceView *m_srv;
	ID3D11RenderTargetView *m_rtv;

	const UINT m_mipLevels;
	const UINT m_arraySize;
	const D3D11_USAGE m_usage;
	const UINT m_bindFlags;
	const UINT m_cpuFlags;
	const UINT m_miscFlags;
	const D3D11_SUBRESOURCE_DATA * const m_initData;
};

class StagingTexture : public Texture
{
public:
	StagingTexture(const D3DDevicesManager &d3d, const UINT width, const UINT height, const DXGI_FORMAT format)
	: Texture(d3d, width, height, format)
	{

	}

	virtual ~StagingTexture()
	{
		
	}

	//sólo debe llamarse a lo sumo una vez por objeto
	virtual HRESULT Init();

	const float *GetMappedData(ID3D11ShaderResourceView *srv) const;
	void CloseMappedData() const;	
};


}

#endif