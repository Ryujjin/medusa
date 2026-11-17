/*
	PrimitiveSurfaceD3D.h
	(c)2005 Palestar, Richard Lyle
	Updated to DirectX 11
*/

#ifndef PRIMITIVESURFACED3D_H
#define PRIMITIVESURFACED3D_H

#include "Display/PrimitiveSurface.h"
#include <d3d11.h>

//---------------------------------------------------------------------------------------------------

class PrimitiveSurfaceD3D : public PrimitiveSurface
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	// Construction
	PrimitiveSurfaceD3D();

	// DevicePrimitive interface
	bool				execute();
	void				clear();
	void				release();
	void				recache();			// Override to ensure resources are released

	// PrimitiveSurface interface
	bool				mipmap() const;
	int					levels() const;
	SizeInt				size() const;
	int					pitch() const;
	RectInt				rectangle() const;
	ColorFormat::Format	colorFormat() const;
	TextureMode			textureMode() const;

	bool				initialize(int width, int height,
		ColorFormat::Format eFormat,
		bool bMipMap = true,
		TextureMode eMode = TM_WRAP);
	void				set(Type eType, int nIndex, int nUV, bool bFiltered, float* pParams = NULL);

	byte* lock(int nLevel = 0);
	bool				unlock();

	// Data
	Type				m_eType;
	int					m_nIndex;
	int					m_nUV;
	bool				m_bFiltered;
	float				m_fParams[4];

	SizeInt				m_Size;
	ColorFormat::Format	m_eFormat;
	bool				m_bMipMap;
	bool				m_bAutoMipMap;		// true = using GenerateMips (non-BC), false = explicit mip uploads
	TextureMode			m_eMode;
	int					m_nLevels;
	int					m_Pitch;

	// DirectX 11 objects
	ID3D11Texture2D*			m_pTexture;			// Main texture (GPU resource)
	ID3D11Texture2D*			m_pStagingTexture;	// Staging texture for Map/Unmap uploads
	ID3D11ShaderResourceView*	m_pSRV;				// Shader Resource View
	int							m_nLockedLevel;		// Currently locked mip level
	bool						m_bLocked;			// Is texture currently locked
	bool						m_bDirty;			// true after first unlock() — main texture has content; preserved on re-lock
	byte*						m_pLockBuffer;		// Mapped staging buffer (or CPU fallback)
	UINT						m_nLockBufferSize;	// Size of lock buffer in bytes
};

//---------------------------------------------------------------------------------------------------

#endif

//---------------------------------------------------------------------------------------------------
// EOF