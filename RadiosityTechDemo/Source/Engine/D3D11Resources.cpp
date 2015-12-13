//------------------------------------------------------------------------------------------
// File: D3D11Resources.cpp
//
// Author: Gabriel Clavero
//------------------------------------------------------------------------------------------


#include "D3D11Resources.h"

namespace DTFramework
{

HRESULT WritableBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"WritableBuffer::Init");
		return E_FAIL;
	}

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	bufferDesc.ByteWidth = m_byteWidth;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;

	HRESULT hr;
	if(FAILED(hr = m_d3dManager.CreateBuffer(&bufferDesc, NULL, &m_buffer))) return hr;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension =	D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = m_numElements;

	if(FAILED(hr = m_d3dManager.CreateShaderResourceView(m_buffer, &srvDesc, &m_srv))) return hr;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = m_format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = m_numElements;

	if(FAILED(hr = m_d3dManager.CreateUnorderedAccessView(m_buffer, &uavDesc, &m_uav))) return hr;

	m_ready = true;

	return hr;
}

HRESULT Texture2D_NOAA::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"Texture2D_NOAA::Init");
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC texdesc;
	texdesc.Width = m_width;
	texdesc.Height = m_height;
	texdesc.MipLevels = m_mipLevels;
	texdesc.ArraySize = m_arraySize;
	texdesc.Format = m_format;
	texdesc.Usage = m_usage;
	texdesc.BindFlags = m_bindFlags;
	texdesc.CPUAccessFlags = m_cpuFlags;
	texdesc.MiscFlags = m_miscFlags;

	texdesc.SampleDesc.Count = 1;
	texdesc.SampleDesc.Quality = 0;

	HRESULT hr;

	if(FAILED( hr = m_d3dManager.CreateTexture2D(&texdesc, m_initData, &m_texture) )) return hr;

	if(m_bindFlags & D3D11_BIND_DEPTH_STENCIL)
	{
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Format =  (m_bindFlags & D3D11_BIND_SHADER_RESOURCE) ? DXGI_FORMAT_D32_FLOAT : m_format;
		dsvDesc.ViewDimension = m_arraySize > 1 ? D3D11_DSV_DIMENSION_TEXTURE2DARRAY : D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = 0;

		if(m_arraySize > 1)
		{
			dsvDesc.Texture2DArray.FirstArraySlice = 0;
			dsvDesc.Texture2DArray.ArraySize = m_arraySize;
			dsvDesc.Texture2DArray.MipSlice = 0;
		}
		else 
		{
			dsvDesc.Texture2D.MipSlice = 0;
		}
		if(FAILED( hr = m_d3dManager.CreateDepthStencilView(m_texture, &dsvDesc, &m_dsv) )) return hr;
	}

	if(m_bindFlags & D3D11_BIND_RENDER_TARGET)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.Format = m_format;
		rtvDesc.ViewDimension = m_arraySize > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DARRAY : D3D11_RTV_DIMENSION_TEXTURE2D;

		if(m_arraySize > 1)
		{
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = m_arraySize;
			rtvDesc.Texture2DArray.MipSlice = 0;
		}
		else
		{
			rtvDesc.Texture2D.MipSlice = 0;
		}

		if(FAILED( hr = m_d3dManager.CreateRenderTargetView( m_texture, &rtvDesc, &m_rtv ))) return hr;
	}

	if(m_bindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		if(m_miscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory( &srvDesc, sizeof( srvDesc ) );
			srvDesc.Format = m_format;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = m_mipLevels;
			srvDesc.TextureCube.MostDetailedMip = 0;

			if(FAILED( hr = m_d3dManager.CreateShaderResourceView(m_texture, &srvDesc, &m_srv) )) return hr;
		}
		else if(m_bindFlags & D3D11_BIND_DEPTH_STENCIL)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
			ZeroMemory( &srvDesc, sizeof( srvDesc ) );
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = m_mipLevels;
			srvDesc.Texture2D.MostDetailedMip = 0;

			if(FAILED( hr = m_d3dManager.CreateShaderResourceView(m_texture, &srvDesc, &m_srv) )) return hr;
		} 
		else
		{
			if(FAILED( hr = m_d3dManager.CreateShaderResourceView(m_texture, NULL, &m_srv) )) return hr;
		}	
	}

	m_ready = true;

	return hr;
}

HRESULT VertexBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"VertexBuffer::Init");
		return E_FAIL;
	}

	HRESULT hr;

	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = m_byteWidth;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	if(m_initData)
	{
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = m_initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		hr = m_d3dManager.CreateBuffer(&desc, &vinitData, &m_buffer);
	}
	else
	{
		hr = m_d3dManager.CreateBuffer(&desc, NULL, &m_buffer);
	}

	if(FAILED(hr)) return hr;

	m_ready = true;

	return hr;
}


HRESULT IndexBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"IndexBuffer::Init");
		return E_FAIL;
	}

	HRESULT hr;

	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = m_byteWidth;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	if(m_initData)
	{
		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = m_initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		hr = m_d3dManager.CreateBuffer(&desc, &vinitData, &m_buffer);
	}
	else
	{
		hr = m_d3dManager.CreateBuffer(&desc, NULL, &m_buffer);
	}

	if(FAILED(hr)) return hr;

	m_ready = true;

	return hr;
}

HRESULT StagingBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingBuffer::Init");
		return E_FAIL;
	}

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = 0;
	bufferDesc.ByteWidth = m_byteWidth;
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.Usage = D3D11_USAGE_STAGING;

	HRESULT hr;
	if(FAILED(hr = m_d3dManager.CreateBuffer(&bufferDesc, NULL, &m_buffer))) return hr;

	m_ready = true;

	return hr;
}

const float *StagingBuffer::GetMappedData(ID3D11Buffer *buf) const
{
	_ASSERT(m_ready && buf);

	if(!m_ready || !buf) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingBuffer::GetMappedData");
		return NULL;
	}

	m_d3dManager.CopyResource( this->m_buffer, buf );

	//map
	D3D11_MAPPED_SUBRESOURCE mapped;
	if(FAILED(m_d3dManager.Map(this->m_buffer, 0, D3D11_MAP_READ, 0, &mapped))) return NULL;

	float *rawVB = reinterpret_cast<float *>(mapped.pData);

	return rawVB;
}

void StagingBuffer::CloseMappedData() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingBuffer::CloseMappedData");
		return;
	}

	m_d3dManager.Unmap(this->m_buffer, 0);
}

HRESULT StagingTexture::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingTexture::Init");
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.BindFlags = 0;
	texDesc.Width = m_width;
	texDesc.Height = m_height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format = m_format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	HRESULT hr;
	if(FAILED(hr = m_d3dManager.CreateTexture2D(&texDesc, NULL, &m_texture))) return  hr;

	m_ready = true;

	return hr;
}

const float *StagingTexture::GetMappedData(ID3D11ShaderResourceView *srv) const
{
	_ASSERT(m_ready && srv);

	if(!m_ready || !srv) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingTexture::GetMappedData");
		return NULL;
	}

	//obtener recurso asociado a ese resource view
	ID3D11Resource *pRes = NULL;
	srv->GetResource(&pRes);

	if(pRes == NULL) return NULL;

	//copiar al staging texture
	m_d3dManager.CopyResource( this->m_texture, pRes );

	//map
	D3D11_MAPPED_SUBRESOURCE mapped;
	if(FAILED(m_d3dManager.Map(this->m_texture, 0, D3D11_MAP_READ, 0, &mapped))) { SAFE_RELEASE(pRes); return NULL; }

	float *rawVB = reinterpret_cast<float *>(mapped.pData);

	SAFE_RELEASE(pRes);

	return rawVB;
}

void StagingTexture::CloseMappedData() const
{
	_ASSERT(m_ready);

	if(!m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"StagingTexture::CloseMappedData");
		return;
	}

	m_d3dManager.Unmap(this->m_texture, 0);
}

HRESULT ConstantBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"ConstantBuffer::Init");
		return E_FAIL;
	}

	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.ByteWidth = m_byteWidth;
	bufferDesc.CPUAccessFlags = m_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;
	bufferDesc.Usage = m_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = m_initData;

	HRESULT hr;
	if(FAILED(hr = m_d3dManager.CreateBuffer(&bufferDesc, m_initData != NULL ? &data : NULL, &m_buffer))) return hr;

	m_ready = true;

	return hr;
}

HRESULT ConstantBuffer::Update(const void * const data, const SIZE_T size)
{
	_ASSERT(m_ready);

	if(!m_ready || !data) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"ConstantBuffer::Update");
		return E_FAIL;
	}

	HRESULT hr;
	D3D11_MAPPED_SUBRESOURCE mappedResource;

	if(FAILED( hr = m_d3dManager.Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource) )) return hr;

	CopyMemory(mappedResource.pData, data, size);

	m_d3dManager.Unmap(m_buffer, 0);

	return S_OK;
}

HRESULT ImmutableBuffer::Init()
{
	_ASSERT(!m_ready);

	if(m_ready) {
		MiscErrorWarning(INVALID_FUNCTION_CALL, L"ImmutableBuffer::Init");
		return E_FAIL;
	}

	HRESULT hr;

	D3D11_BUFFER_DESC desc;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = m_byteWidth;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = m_initData;
	vinitData.SysMemPitch = 0;
	vinitData.SysMemSlicePitch = 0;

	if(FAILED(hr = m_d3dManager.CreateBuffer(&desc, &vinitData, &m_buffer))) return hr;

	//ahora crear el srv
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = m_format;
	srvDesc.ViewDimension =	D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.ElementOffset = 0;
	srvDesc.Buffer.ElementWidth = m_elementWidth;

	if(FAILED(hr = m_d3dManager.CreateShaderResourceView(m_buffer, &srvDesc, &m_srv))) return hr;

	m_ready = true;

	return hr;
}

}