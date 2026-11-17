/*
	PrimitiveSurfaceD3D.cpp
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#include "Debug/Trace.h"
#include "Standard/Bits.h"
#include "DisplayDeviceD3D.h"
#include "PrimitiveSurfaceD3D.h"
#include <new>  // for std::nothrow

#define ENABLE_TEXTURE_COMPRESSION		0

//---------------------------------------------------------------------------------------------------

#include "PrimitiveFactory.h"

IMPLEMENT_DEVICE_PRIMITIVE(PrimitiveSurfaceD3D);
IMPLEMENT_PRIMITIVE_FACTORY(PrimitiveSurfaceD3D);

PrimitiveSurfaceD3D::PrimitiveSurfaceD3D() :
	m_eType(DIFFUSE),
	m_nIndex(0),
	m_nUV(0),
	m_bFiltered(true),
	m_Size(0, 0),
	m_eFormat(ColorFormat::INVALID),
	m_bMipMap(true),
	m_bAutoMipMap(false),
	m_eMode(TM_WRAP),
	m_nLevels(0),
	m_Pitch(0),
	m_pTexture(NULL),
	m_pStagingTexture(NULL),
	m_pSRV(NULL),
	m_nLockedLevel(0),
	m_bLocked(false),
	m_bDirty(false),
	m_pLockBuffer(NULL),
	m_nLockBufferSize(0)
{
	memset(m_fParams, 0, sizeof(m_fParams));
}

//------------------------------------------------------------------------------------

bool PrimitiveSurfaceD3D::execute()
{
	DisplayDeviceD3D* pDisplayD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDisplayD3D || !pDisplayD3D->getContext())
		return false;

	if (!m_pSRV)
		return false;

	ID3D11DeviceContext* pContext = pDisplayD3D->getContext();
	int nTextureSlot = 0;

	// DX11 doesn't use fixed-function pipeline, all rendering is shader-based
	// Set the texture to the appropriate shader resource slot
	switch (m_eType)
	{
	case DIFFUSE:
	{
		nTextureSlot = 0;
		pContext->PSSetShaderResources(nTextureSlot, 1, &m_pSRV);

		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->setConstant("bEnableDiffuse", true);
	}
	break;

	case LIGHTMAP:
	{
		// check if lightmaps have been disabled
		if (!sm_bEnableLightMaps)
			break;

		nTextureSlot = 1;
		pContext->PSSetShaderResources(nTextureSlot, 1, &m_pSRV);

		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->setConstant("bEnableLightMap", true);
	}
	break;

	case BUMPMAP:
	{
		if (!sm_bEnableBumpMaps)
			break;

		nTextureSlot = 2;
		pContext->PSSetShaderResources(nTextureSlot, 1, &m_pSRV);

		if (pDisplayD3D->m_pMatShader.valid())
		{
			pDisplayD3D->m_pMatShader->setConstant("bEnableBumpMap", true);
			if (m_fParams[0] > 0.0f)
				pDisplayD3D->m_pMatShader->setConstant("fBumpDepth", m_fParams[0]);
			else
				pDisplayD3D->m_pMatShader->clearConstant("fBumpDepth");
		}
	}
	break;

	case NORMALMAP:
	{
		nTextureSlot = 3;
		pContext->PSSetShaderResources(nTextureSlot, 1, &m_pSRV);

		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->setConstant("bEnableNormalMap", true);
	}
	break;

	case GLOSSMAP:
	{
		nTextureSlot = 4;
		pContext->PSSetShaderResources(nTextureSlot, 1, &m_pSRV);

		if (pDisplayD3D->m_pMatShader.valid())
			pDisplayD3D->m_pMatShader->setConstant("bEnableGlossMap", true);
	}
	break;

	case DARKMAP:
	case DETAILMAP:
	case PARALLAXMAP:
	case DECALMAP:
	case SHADERMAP:
		// Not yet implemented
		break;
	}

	// Set sampler state based on filtering and texture mode
	ID3D11SamplerState* pSampler = NULL;

	if (m_bFiltered)
	{
		switch (pDisplayD3D->m_eFilterMode)
		{
		case DisplayDevice::FILTER_ANISOTROPIC:
			pSampler = pDisplayD3D->m_pSamplerAnisotropic.Get();
			break;
		case DisplayDevice::FILTER_LINEAR:
			pSampler = pDisplayD3D->m_pSamplerLinear.Get();
			break;
		default:
			pSampler = pDisplayD3D->m_pSamplerLinear.Get();
			break;
		}
	}
	else
	{
		pSampler = pDisplayD3D->m_pSamplerPoint.Get();
	}

	if (pSampler)
		pContext->PSSetSamplers(nTextureSlot, 1, &pSampler);

	return true;
}

void PrimitiveSurfaceD3D::clear()
{
	// Nothing to clear in DX11 - resources are managed differently
}

void PrimitiveSurfaceD3D::release()
{
	try {
		// Free CPU lock buffer if still allocated
		delete[] m_pLockBuffer;
		m_pLockBuffer = NULL;
		m_nLockBufferSize = 0;
		m_bLocked = false;
		m_bDirty = false;

		// Release all GPU resources
		RELEASEDD(m_pSRV);
		RELEASEDD(m_pStagingTexture);
		RELEASEDD(m_pTexture);
	}
	catch (...)
	{
		m_pSRV = NULL;
		m_pStagingTexture = NULL;
		m_pTexture = NULL;
		m_pLockBuffer = NULL;
		m_nLockBufferSize = 0;
		m_bLocked = false;
	}
}

void PrimitiveSurfaceD3D::recache()
{
	// Called when the last reference is released
	// Release all D3D11 resources to free memory
	release();
}

//------------------------------------------------------------------------------------

bool PrimitiveSurfaceD3D::mipmap() const
{
	return m_bMipMap;
}

int PrimitiveSurfaceD3D::levels() const
{
	return m_nLevels;
}

SizeInt PrimitiveSurfaceD3D::size() const
{
	return m_Size;
}

int PrimitiveSurfaceD3D::pitch() const
{
	return m_Pitch;
}

RectInt PrimitiveSurfaceD3D::rectangle() const
{
	return RectInt(0, 0, m_Size);
}

ColorFormat::Format PrimitiveSurfaceD3D::colorFormat() const
{
	return m_eFormat;
}

PrimitiveSurface::TextureMode PrimitiveSurfaceD3D::textureMode() const
{
	return m_eMode;
}

//------------------------------------------------------------------------------------

inline int getMipLevels(int width, int height)
{
	return GetLastBit(Min(width, height));
}

bool PrimitiveSurfaceD3D::initialize(int width, int height, Format eFormat, bool bMipMap, TextureMode eMode)
{
	// release the previous surface
	release();

	// check for bad parameters
	if (width <= 0 || height <= 0)
		return false;

	DisplayDeviceD3D* pDeviceD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDeviceD3D || !pDeviceD3D->m_pDevice)
		return false;

	// initialize the new surface
	m_Size = SizeInt(width, height);
	m_eFormat = eFormat;
	m_bMipMap = bMipMap;
	m_eMode = eMode;
	m_Pitch = 0;

	// Calculate mip levels
	// For DX11 with D3D11_RESOURCE_MISC_GENERATE_MIPS, we use 0 to generate full mip chain
	if (bMipMap)
	{
		m_nLevels = 0;  // 0 means generate full mip chain automatically
	}
	else
	{
		m_nLevels = 1;
	}

	// Create texture description
	// For DX11, use D3D11_USAGE_DEFAULT to support automatic mipmap generation
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = width;
	texDesc.Height = height;

	texDesc.MipLevels = m_nLevels;  // 0 for full mip chain, 1 for no mipmaps
	texDesc.ArraySize = 1;
	texDesc.Format = DisplayDeviceD3D::findFormat(m_eFormat);
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;

	// For mipmapped textures, decide between auto-generation and explicit mip uploads.
	// Block-compressed (BC/DXT) formats CANNOT be render targets in DX11, so
	// D3D11_RESOURCE_MISC_GENERATE_MIPS is not available for them.
	// Instead, BC textures upload each pre-computed mip level explicitly via lock/unlock.
	bool bBlockCompressed = (texDesc.Format >= DXGI_FORMAT_BC1_TYPELESS && texDesc.Format <= DXGI_FORMAT_BC5_SNORM)
		|| (texDesc.Format >= DXGI_FORMAT_BC6H_TYPELESS && texDesc.Format <= DXGI_FORMAT_BC7_UNORM_SRGB);

	m_bAutoMipMap = bMipMap && !bBlockCompressed;
	if (m_bAutoMipMap)
	{
		texDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		texDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
	}

	// Create the texture
	HRESULT hr = S_OK;
	try
	{
		hr = pDeviceD3D->m_pDevice->CreateTexture2D(&texDesc, NULL, &m_pTexture);
	}
	catch (...)
	{
		hr = E_FAIL;
		LOG_ERROR("PrimitiveSurfaceD3D", "Exception caught during CreateTexture2D!");
	}

	if (FAILED(hr))
	{
		m_pTexture = NULL;  // Ensure pointer is NULL on failure
		LOG_ERROR("PrimitiveSurfaceD3D", CharString().format("CreateTexture2D failed HR=0x%08X Size:%dx%d Format:%d MipMap:%d",
			hr, width, height, m_eFormat, bMipMap));
		return false;
	}

	static int s_nTextureCount = 0;
	static int s_nTextureReleaseCount = 0;
	s_nTextureCount++;

	// Safety check: warn if too many active textures
	int nActiveTextures = s_nTextureCount - s_nTextureReleaseCount;
	if (nActiveTextures > 500)
	{
		LOG_WARNING("PrimitiveSurfaceD3D", "WARNING: %d active textures! Possible memory leak or excessive creation.", nActiveTextures);
	}

	// Estimate memory usage (very rough)
	// Assume 4 bytes per pixel for most formats, multiply by mip chain overhead (1.33x)
	int estimatedBytes = width * height * 4;
	if (m_nLevels > 1 || m_nLevels == 0) // If mipmapped
		estimatedBytes = (int)(estimatedBytes * 1.33f);

	// Get the actual mip level count that was created
	D3D11_TEXTURE2D_DESC createdDesc;
	m_pTexture->GetDesc(&createdDesc);
	m_nLevels = createdDesc.MipLevels;

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = -1;  // -1 means use all mip levels
	srvDesc.Texture2D.MostDetailedMip = 0;

	try
	{
		hr = pDeviceD3D->m_pDevice->CreateShaderResourceView(m_pTexture, &srvDesc, &m_pSRV);
	}
	catch (...)
	{
		hr = E_FAIL;
		LOG_ERROR("PrimitiveSurfaceD3D", "Exception caught during CreateShaderResourceView!");
	}

	if (FAILED(hr))
	{
		m_pSRV = NULL;  // Ensure pointer is NULL on failure
		LOG_ERROR("PrimitiveSurfaceD3D", CharString().format("CreateShaderResourceView failed HR=0x%08X", hr));
		RELEASEDD(m_pTexture);
		m_pTexture = NULL;  // Also NULL the texture pointer for consistency
		return false;
	}

	return true;
}

void PrimitiveSurfaceD3D::set(Type eType, int nIndex, int nUV, bool bFiltered, float* pParams)
{
	m_eType = eType;
	m_nIndex = nIndex;
	m_nUV = nUV;
	m_bFiltered = bFiltered;

	if (pParams != NULL)
		memcpy(m_fParams, pParams, sizeof(m_fParams));
}

byte* PrimitiveSurfaceD3D::lock(int nLevel)
{
	if (!m_pTexture)
		return NULL;

	if (m_bLocked)
		unlock();

	DisplayDeviceD3D* pDeviceD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDeviceD3D || !pDeviceD3D->m_pContext)
		return NULL;

	// For auto-generated mipmaps (non-BC formats), only level 0 can be locked;
	// GenerateMips() fills the rest. For BC formats with explicit mipmaps,
	// all levels must be lockable to upload pre-computed mip data.
	if (m_bAutoMipMap && nLevel != 0)
		return NULL;

	// Clean up any previous CPU fallback buffer (safe: unlock() NULLs m_pLockBuffer in both paths)
	if (m_pLockBuffer && !m_pStagingTexture)
		delete[] m_pLockBuffer;
	m_pLockBuffer = NULL;
	m_nLockBufferSize = 0;

	// Get the description from the main texture
	D3D11_TEXTURE2D_DESC mainDesc;
	m_pTexture->GetDesc(&mainDesc);

	// Calculate dimensions for the requested mip level
	UINT mipWidth = Max(1u, mainDesc.Width >> nLevel);
	UINT mipHeight = Max(1u, mainDesc.Height >> nLevel);

	// PRIMARY: Use a staging texture with Map/Unmap.
	// The GPU driver allocates staging memory from its own pool,
	// which does NOT consume C++ heap address space - critical for 32-bit builds.
	// Reuse existing staging texture if dimensions/format match (unlock() keeps it alive).
	if (m_pStagingTexture)
	{
		D3D11_TEXTURE2D_DESC existingDesc;
		m_pStagingTexture->GetDesc(&existingDesc);
		if (existingDesc.Width != mipWidth || existingDesc.Height != mipHeight || existingDesc.Format != mainDesc.Format
			|| !(existingDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ))
		{
			m_pStagingTexture->Release();
			m_pStagingTexture = NULL;
		}
	}

	// Track whether staging is freshly created this call (vs. reused from a previous lock).
	// When reusing, we copy the main texture back to staging so prior pixel data is preserved.
	// This matches the DX9 lock() semantic where existing surface content was returned.
	bool bFreshStaging = (m_pStagingTexture == NULL);

	if (!m_pStagingTexture)
	{
		D3D11_TEXTURE2D_DESC stagingDesc = {};
		stagingDesc.Width = mipWidth;
		stagingDesc.Height = mipHeight;
		stagingDesc.MipLevels = 1;
		stagingDesc.ArraySize = 1;
		stagingDesc.Format = mainDesc.Format;
		stagingDesc.SampleDesc.Count = 1;
		stagingDesc.SampleDesc.Quality = 0;
		stagingDesc.Usage = D3D11_USAGE_STAGING;
		stagingDesc.BindFlags = 0;
		stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		stagingDesc.MiscFlags = 0;

		HRESULT hr = pDeviceD3D->m_pDevice->CreateTexture2D(&stagingDesc, NULL, &m_pStagingTexture);
		if (FAILED(hr))
			m_pStagingTexture = NULL;
	}

	if (m_pStagingTexture)
	{
		// If this surface has been written before and we're reusing the same staging texture,
		// copy the current main-texture content into staging first so the caller sees
		// existing pixel data.  This matches DX9 lock() semantics and is required for
		// the font atlas builder, which calls Draw::draw() once per glyph (each a separate
		// lock/unlock), accumulating all glyphs into one texture.
		// For a freshly created staging texture (bFreshStaging) or a never-written surface
		// (!m_bDirty), we skip the copy and zero the buffer instead.
		bool bPreserveData = (m_bDirty && !bFreshStaging);
		if (bPreserveData)
		{
			D3D11_TEXTURE2D_DESC mainDescCopy;
			m_pTexture->GetDesc(&mainDescCopy);
			pDeviceD3D->m_pContext->CopySubresourceRegion(
				m_pStagingTexture, 0, 0, 0, 0,
				m_pTexture, D3D11CalcSubresource(nLevel, 0, mainDescCopy.MipLevels),
				NULL);
		}

		D3D11_MAP mapType = bPreserveData ? D3D11_MAP_READ_WRITE : D3D11_MAP_WRITE;
		D3D11_MAPPED_SUBRESOURCE mapped;
		HRESULT hr = pDeviceD3D->m_pContext->Map(m_pStagingTexture, 0, mapType, 0, &mapped);
		if (SUCCEEDED(hr))
		{
			m_Pitch = mapped.RowPitch;
			m_nLockBufferSize = mapped.DepthPitch;
			m_pLockBuffer = (byte*)mapped.pData;
			if (!bPreserveData)
				memset(m_pLockBuffer, 0, m_nLockBufferSize);  // Zero fresh/clean surface only
			m_nLockedLevel = nLevel;
			m_bLocked = true;

			return m_pLockBuffer;
		}
		// Map failed, release staging texture and fall through to CPU fallback
		m_pStagingTexture->Release();
		m_pStagingTexture = NULL;
	}

	// FALLBACK: allocate CPU-side buffer if staging texture failed
	UINT rowCount = mipHeight;
	if (mainDesc.Format >= DXGI_FORMAT_BC1_TYPELESS && mainDesc.Format <= DXGI_FORMAT_BC5_SNORM)
	{
		// Block-compressed formats use 4x4 pixel blocks
		UINT numBlocksWide = Max(1u, (mipWidth + 3) / 4);
		rowCount = Max(1u, (mipHeight + 3) / 4);
		UINT bytesPerBlock = ((mainDesc.Format >= DXGI_FORMAT_BC1_TYPELESS && mainDesc.Format <= DXGI_FORMAT_BC1_UNORM_SRGB)
			|| (mainDesc.Format >= DXGI_FORMAT_BC4_TYPELESS && mainDesc.Format <= DXGI_FORMAT_BC4_SNORM)) ? 8 : 16;
		m_Pitch = numBlocksWide * bytesPerBlock;
	}
	else
	{
		UINT bpp = DisplayDeviceD3D::getFormatDepth(mainDesc.Format);
		if (bpp == 0) bpp = 32;
		m_Pitch = mipWidth * (bpp / 8);
	}
	m_nLockBufferSize = m_Pitch * rowCount;

	m_pLockBuffer = new(std::nothrow) byte[m_nLockBufferSize];
	if (!m_pLockBuffer)
	{
		m_nLockBufferSize = 0;
		return NULL;
	}

	memset(m_pLockBuffer, 0, m_nLockBufferSize);
	m_nLockedLevel = nLevel;
	m_bLocked = true;

	return m_pLockBuffer;
}

bool PrimitiveSurfaceD3D::unlock()
{
	if (!m_bLocked)
		return true;

	if (!m_pTexture)
		return false;

	DisplayDeviceD3D* pDeviceD3D = (DisplayDeviceD3D*)m_pDevice;
	if (!pDeviceD3D || !pDeviceD3D->m_pContext)
		return false;

	if (m_pStagingTexture)
	{
		// Staging texture path: Unmap + CopySubresourceRegion
		pDeviceD3D->m_pContext->Unmap(m_pStagingTexture, 0);

		D3D11_TEXTURE2D_DESC mainDesc;
		m_pTexture->GetDesc(&mainDesc);

		pDeviceD3D->m_pContext->CopySubresourceRegion(
			m_pTexture,
			D3D11CalcSubresource(m_nLockedLevel, 0, mainDesc.MipLevels),
			0, 0, 0,
			m_pStagingTexture,
			0,
			NULL
		);

		// Keep staging texture alive for reuse by subsequent lock() calls
		m_pLockBuffer = NULL;  // Was pointing into mapped staging memory
		m_bDirty = true;       // Main texture now has content; preserve on next lock()
	}
	else if (m_pLockBuffer)
	{
		// CPU buffer fallback path: UpdateSubresource
		D3D11_TEXTURE2D_DESC mainDesc;
		m_pTexture->GetDesc(&mainDesc);

		pDeviceD3D->m_pContext->UpdateSubresource(
			m_pTexture,
			D3D11CalcSubresource(m_nLockedLevel, 0, mainDesc.MipLevels),
			NULL,
			m_pLockBuffer,
			m_Pitch,
			0
		);

		delete[] m_pLockBuffer;
		m_pLockBuffer = NULL;
		m_bDirty = true;       // Main texture now has content; preserve on next lock()
	}

	// Generate mipmaps only for auto-mipmap textures (non-BC uncompressed formats).
	// BC textures have pre-computed mip data uploaded explicitly per level.
	if (m_bAutoMipMap && m_nLockedLevel == 0 && m_pSRV)
	{
		pDeviceD3D->m_pContext->GenerateMips(m_pSRV);
	}

	m_bLocked = false;
	return true;
}

//------------------------------------------------------------------------------------
// EOF
