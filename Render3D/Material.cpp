/*
	Material.cpp

	Material Resource
	(c)2005 Palestar, Richard Lyle
*/

#define RENDER3D_DLL
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters in the browser information

#include "Debug/Assert.h"
#include "Debug/Trace.h"
#include "Standard/Bits.h" 
#include "Standard/Limits.h" 
#include "Draw/Draw.h"
#include "File/FileDisk.h"
#include "Render3D/Material.h"

//---------------------------------------------------------------------------------------------------

CriticalSection				Material::sm_SurfaceHashLock;
Material::SurfaceHash		Material::sm_SurfaceHash;
SizeInt						Material::sm_MaxTextureSize( 4096, 4096 );			// max allowable texture size
CriticalSection				Material::sm_PendingLock;
Array<Material::Ref>		Material::sm_PendingMaterials;

// Register the render-thread flush callback with DisplayDevice so that
// DisplayDeviceD3D::lock() can drain pending material surfaces every frame
// without a circular dependency on the Render3D library.
static void s_FlushPendingMaterials( DisplayDevice * pDevice )
{
	Material::flushPendingDeviceSurfaces( pDevice );
}
static struct MaterialCallbackRegistrar
{
	MaterialCallbackRegistrar()  { DisplayDevice::sm_pfnFlushPendingMaterials = &s_FlushPendingMaterials; }
	~MaterialCallbackRegistrar() { DisplayDevice::sm_pfnFlushPendingMaterials = NULL; }
} s_MaterialCallbackRegistrar;

//--- Texture disk cache helpers ---
// Cache format: TxcHeader followed by N level entries of (int dataSize, byte data[dataSize]).
// Keyed by the surface WidgetKey, stored under <exeDir>\TextureCache\.
// On second+ run, surfaces are rebuilt from cache, skipping Image format conversion and
// mip generation entirely.  Analogous to the .cso shader bytecode cache in D3d10shader.cpp.

#pragma pack(push, 1)
struct TxcHeader {
	char  magic[4];		// "TXC1"
	int   version;		// 1
	int   baseWidth;
	int   baseHeight;
	int   format;		// ColorFormat::Format (int)
	int   numLevels;	// mip levels stored
};
#pragma pack(pop)

static CharString s_BuildTexCachePath( const WidgetKey & key )
{
	static CharString s_sHome;
	if ( s_sHome.length() == 0 )
	{
		s_sHome = FileDisk::home();
		FileDisk::createDirectory( s_sHome + "\\TextureCache" );
	}
	char szFile[32];
	sprintf_s( szFile, sizeof(szFile), "%016I64x.txc", (unsigned __int64)(qword)key );
	return s_sHome + "\\TextureCache\\" + szFile;
}

// Returns true and fills pSurface if the cache file is valid.
// Returns false (pSurface unchanged) on any miss or error so the caller falls through to full creation.
static bool s_LoadTexCache( const CharString & sPath, DisplayDevice * pDisplay,
							PrimitiveSurface::Ref & pSurface, bool bMipMap )
{
	if ( !FileDisk::fileExists( sPath ) )
		return false;

	FileDisk f;
	if ( !f.open( sPath, FileDisk::READ, false ) )
		return false;

	TxcHeader hdr;
	if ( (int)f.read( &hdr, sizeof(hdr) ) != (int)sizeof(hdr) )
		return false;
	if ( memcmp( hdr.magic, "TXC1", 4 ) != 0 || hdr.version != 1 )
		return false;
	if ( hdr.baseWidth <= 0 || hdr.baseHeight <= 0 || hdr.numLevels <= 0 )
		return false;

	pDisplay->create( pSurface );
	if ( !pSurface->initialize( hdr.baseWidth, hdr.baseHeight,
		(ColorFormat::Format)hdr.format, bMipMap ) )
	{
		pSurface = NULL;
		return false;
	}

	const bool bBC = ( hdr.format == (int)ColorFormat::DXT1 ||
					   hdr.format == (int)ColorFormat::DXT3 ||
					   hdr.format == (int)ColorFormat::DXT5 );
	bool bOk = true;
	for ( int i = 0; i < hdr.numLevels && bOk; ++i )
	{
		int dataSize = 0;
		if ( (int)f.read( &dataSize, sizeof(int) ) != (int)sizeof(int) || dataSize <= 0 )
		{
			bOk = false;
			break;
		}
		byte * pData = new byte[ dataSize ];
		if ( (int)f.read( pData, dataSize ) != dataSize )
		{
			delete[] pData;
			bOk = false;
			break;
		}

		void * pPixels = pSurface->lock( i );
		if ( !pPixels )
		{
			delete[] pData;
			bOk = false;
			break;
		}

		// Reconstruct srcPitch from cached data size + level dimensions
		const int mipHeight = Max( 1, hdr.baseHeight >> i );
		const int numRows   = bBC ? Max( 1, (mipHeight + 3) / 4 ) : mipHeight;
		const int srcPitch  = (numRows > 0) ? (dataSize / numRows) : dataSize;
		const int dstPitch  = pSurface->pitch();

		if ( srcPitch == dstPitch )
		{
			memcpy( pPixels, pData, dataSize );
		}
		else
		{
			const byte * pSrc = pData;
			byte * pDst = (byte *)pPixels;
			for ( int row = 0; row < numRows; ++row )
			{
				memcpy( pDst, pSrc, srcPitch );
				pSrc += srcPitch;
				pDst += dstPitch;
			}
		}
		pSurface->unlock();
		delete[] pData;
	}

	if ( !bOk )
	{
		TRACE( "TextureCache: corrupt cache entry, deleting and falling back to full load" );
		pSurface = NULL;
		FileDisk::deleteFile( sPath );
		return false;
	}

	return true;
}

// Write processed Image pixel data (post-resize/reformat) to the disk cache.
// pImage must be the final processed image (may be a copy created for resize/reformat).
static void s_SaveTexCache( const CharString & sPath, Image * pImage,
							int nFrame, ColorFormat::Format eFormat, int nLevels )
{
	if ( !pImage || nLevels <= 0 )
		return;

	// Count how many mip levels the image chain actually has — may be less than
	// pSurface->levels() if the image was built with a shorter chain. Writing a header
	// that claims more levels than are actually stored causes s_LoadTexCache to hit EOF
	// mid-read and incorrectly treat the file as corrupt.
	int nActualLevels = 0;
	for ( Image * pCount = pImage; pCount && nActualLevels < nLevels; pCount = pCount->mipMap() )
		++nActualLevels;

	TxcHeader hdr;
	memcpy( hdr.magic, "TXC1", 4 );
	hdr.version    = 1;
	hdr.baseWidth  = pImage->size().width;
	hdr.baseHeight = pImage->size().height;
	hdr.format     = (int)eFormat;
	hdr.numLevels  = nActualLevels;

	FileDisk f;
	if ( !f.open( sPath, FileDisk::WRITE, false ) )
		return;

	f.write( &hdr, sizeof(hdr) );

	Image * pMip = pImage;
	for ( int i = 0; i < nActualLevels && pMip; ++i )
	{
		const dword dataSize = pMip->frameSize( nFrame );
		const int iDataSize  = (int)dataSize;
		f.write( &iDataSize, sizeof(int) );
		f.write( pMip->frame( nFrame ), dataSize );
		pMip = pMip->mipMap();
	}
	f.flush();
	f.close();

	TRACE( CharString().format( "TextureCache: wrote %s (%dx%d fmt=%d lvl=%d)",
		(const char *)sPath, hdr.baseWidth, hdr.baseHeight, hdr.format, hdr.numLevels ) );
}

// Deferred save queue — first-run .txc writes are pushed here during getSurface()
// and drained at 2/frame inside flushPendingDeviceSurfaces() to avoid blocking
// the render thread with burst disk I/O on the first load.
struct TxcSaveItem
{
	CharString			path;
	Image::Link			pImage;
	int					nFrame;
	ColorFormat::Format	eFormat;
	int					nLevels;
};

static CriticalSection		s_TxcSaveLock;
static Array<TxcSaveItem>	s_TxcSaveQueue;

static void s_QueueSaveTexCache( const CharString & sPath, Image * pImage,
								 int nFrame, ColorFormat::Format eFormat, int nLevels )
{
	TxcSaveItem item;
	item.path    = sPath;
	item.pImage  = pImage;
	item.nFrame  = nFrame;
	item.eFormat = eFormat;
	item.nLevels = nLevels;
	AutoLock lock( &s_TxcSaveLock );
	s_TxcSaveQueue.push( item );
}

static void s_DrainSaveQueue()
{
	static const int MAX_SAVES_PER_DRAIN = 2;
	for ( int i = 0; i < MAX_SAVES_PER_DRAIN; ++i )
	{
		TxcSaveItem item;
		{
			AutoLock lock( &s_TxcSaveLock );
			if ( s_TxcSaveQueue.size() == 0 )
				return;
			item = s_TxcSaveQueue[0];
			s_TxcSaveQueue.remove( 0 );
		}
		s_SaveTexCache( item.path, item.pImage, item.nFrame, item.eFormat, item.nLevels );
	}
}

//----------------------------------------------------------------------------

BEGIN_PROPERTY_LIST( Material, Resource )
	ADD_PROPERTY( m_Flammable );
	ADD_PROPERTY( m_Hardness );
	ADD_PROPERTY( m_Strength );
	ADD_PROPERTY( m_Friction );
	ADD_PROPERTY( m_nPass );
	ADD_PROPERTY( m_Width );
	ADD_PROPERTY( m_Height );
	ADD_PROPERTY( m_Diffuse );
	ADD_PROPERTY( m_Ambient );
	ADD_PROPERTY( m_Emissive );
	ADD_PROPERTY( m_Specular );
	ADD_PROPERTY( m_SpecularPower );
	ADD_PROPERTY( m_LightEnable );
	ADD_PROPERTY( m_Blending );
	ADD_PROPERTY( m_DoubleSided );
	ADD_PROPERTY( m_Fps );
	ADD_PROPERTY( m_Frames );
	ADD_PROPERTY( m_sShader );
	ADD_PROPERTY( m_Textures );
	// deprecated properties
	ADD_PROPERTY( m_DiffuseTexture );
	ADD_PROPERTY( m_LightMap );
	ADD_PROPERTY( m_MipMap );
END_PROPERTY_LIST()

//----------------------------------------------------------------------------

IMPLEMENT_RESOURCE_FACTORY( Material, Resource );

Material::Material()
{
	initialize();
}

Material::Material( Blending blend /*= PrimitiveMaterial::NONE*/ )
{
	initialize();

	m_Blending = blend;
	m_nPass = (blend == PrimitiveMaterial::NONE) ? 1 : 2;
}

Material::Material( Image * pDiffuse, Blending blend /*= PrimitiveMaterial::NONE*/ )
{
	initialize();

	m_Textures.push( Texture( PrimitiveSurface::DIFFUSE, pDiffuse ) );
	m_Blending = blend;
	m_nPass = (blend == PrimitiveMaterial::NONE) ? 1 : 2;
}

Material::Material( Color color, bool emissive /*= true*/, Blending blend /*= PrimitiveMaterial::NONE*/ )
{
	initialize();

	m_Diffuse = color;				// lighting properties
	m_Ambient = color;
	m_Emissive = emissive ? color : Color(0,0,0);
	m_Specular = color;
	m_Blending = blend;
	m_nPass = (blend == PrimitiveMaterial::NONE) ? 1 : 2;
}

Material::Material( Color diffuse, Color ambient, Color emissive, Blending blend /*= PrimitiveMaterial::NONE*/ )
{
	initialize();

	m_Diffuse = diffuse;				// lighting properties
	m_Ambient = ambient;
	m_Emissive = emissive;
	m_Blending = blend;
	m_nPass = (blend == PrimitiveMaterial::NONE) ? 1 : 2;
}

Material::~Material()
{}

//---------------------------------------------------------------------------------------------------

bool Material::read( const InStream & input )
{
	if (! Resource::read( input ) )
		return false;

	// handle deprecated properties..
	if ( m_DiffuseTexture.valid() )
	{
		m_Textures.push( Texture( PrimitiveSurface::DIFFUSE, m_DiffuseTexture ) );
		m_DiffuseTexture = NULL;
	}
	if ( m_LightMap.valid() )
	{
		m_Textures.push( Texture( PrimitiveSurface::LIGHTMAP, m_LightMap ) );
		m_LightMap = NULL;
	}

	// NOTE: Do NOT call createDeviceSurfaces() here.  Material::read() may be invoked
	// from Broker::loadingThread (a background thread), and createDeviceSurfaces() calls
	// PrimitiveSurfaceD3D::lock() which uses the D3D11 immediate context.  The immediate
	// context is not thread-safe; concurrent access from the render thread (Flush, Draw,
	// etc.) causes a driver-level deadlock on NVIDIA.
	// Instead, queue this material so the render thread processes it during its next frame
	// (via flushPendingDeviceSurfaces in createDevicePrimitives).  This restores the DX9
	// behaviour where texture uploads overlapped with Broker disk I/O.
	if ( DisplayDevice::sm_pCacheDevice != NULL )
	{
		AutoLock lock( &sm_PendingLock );
		sm_PendingMaterials.push( Ref(this) );
	}

	return true;
}

//----------------------------------------------------------------------------

void Material::setPhysical( float flammable, float hardness, float strength, float friction,
						   float width, float height )
{
	m_Flammable = flammable;
	m_Hardness = hardness;
	m_Strength = strength;
	m_Friction = friction;
	m_Width = width;
	m_Height = height;
}

void Material::setPass( int nPass )
{
	m_nPass = nPass;
}

void Material::setLighting( Color diffuse, Color ambient, Color emissive, Color specular, float specularPower )
{
	m_Diffuse = diffuse;
	m_Ambient = ambient;
	m_Emissive = emissive;
	m_Specular = specular;
	m_SpecularPower = specularPower;

	m_Material = NULL;
}

void Material::setLightEnable( bool enable )
{
	m_LightEnable = enable;
	m_Material = NULL;
}

void Material::setBlending( Blending blending )
{
	m_Blending = blending;
	m_Material = NULL;
}

void Material::setDoubleSided( bool doubleSided )
{
	m_DoubleSided = doubleSided;
	m_Material = NULL;
}

void Material::setFps( float fps )
{
	m_Fps = fps;
}

void Material::setFrames( int frames )
{
	ASSERT( frames >= 1 );
	m_Frames = frames;
}

void Material::setShader( const char * pShader )
{
	m_sShader = pShader;
	m_Material = NULL;
}

bool Material::addTexture( const Texture & texture )
{
	if (! texture.m_pImage.valid() )
		return false;

	m_Textures.push( texture );
	return true;
}

bool Material::removeTexture( int n )
{
	if ( n < 0 || n >= m_Textures.size() )
		return false;
	m_Textures.remove( n );
	return true;
}

DevicePrimitive * Material::material( RenderContext & context )
{
	// First-time creation or alpha change: rebuild the full D3D primitive.
	if (! m_Material.valid() || m_fLastAlpha != context.alpha() )
		createDevicePrimitives( context );
	else if ( !m_bSurfaceReady )
	{
		// Primitive already exists but some surfaces were not yet loaded on the previous
		// frame.  Retry surface creation only — do NOT rebuild the whole material primitive,
		// as that floods addSurface with NULLs and causes per-frame D3D state churn.
		createDeviceSurfaces( context.display() );
		if ( m_bSurfaceReady && m_Surfaces.size() > 0 )
		{
			m_Material->clearSurfaces();
			Array< PrimitiveSurface::Ref > & surfaces = m_Surfaces[ 0 ];
			for ( int i = 0; i < surfaces.size(); ++i )
			{
				if ( !surfaces[i].valid() )
					continue;
				Texture & texture = m_Textures[ i ];
				m_Material->addSurface( surfaces[i],
					texture.m_eType,
					texture.m_nIndex,
					texture.m_nUV,
					texture.m_fParams );
			}
			m_nLastFrame = 0;
		}
	}

	// handle animating surfaces
	if ( m_Frames > 1 )
	{
		int nFrame = int(context.time() * m_Fps) % m_Frames;
		if ( nFrame < 0 )
			nFrame = -nFrame;

		if ( nFrame != m_nLastFrame )
		{
			// remove the old textures from the material
			m_Material->clearSurfaces();

			if ( m_Surfaces.size() > 0 )
			{
				Array< PrimitiveSurface::Ref > & surfaces = m_Surfaces[ nFrame % m_Surfaces.size() ];
				ASSERT( surfaces.size() == m_Textures.size() );

				for(int i=0;i<surfaces.size();++i)
					{
						Texture & texture = m_Textures[ i ];
						if ( !surfaces[i].valid() )
							continue;
						m_Material->addSurface( surfaces[i], 
							texture.m_eType,
							texture.m_nIndex, 
							texture.m_nUV, 
							texture.m_fParams );
					}
			}

			m_nLastFrame = nFrame;
		}
	}

	return m_Material;
}

void Material::flush()
{
	m_Material = NULL;
	m_Surfaces.release();
}

void Material::flushMaterial()
{
	m_Material = NULL;
}

//-------------------------------------------------------------------------------

void Material::push( RenderContext & context, Material * pMaterial )
{
	if ( pMaterial != NULL )
		context.push( pMaterial->material( context ) );
}

void Material::push( RenderContext & context, Color color )
{
	context.push( Material( color ).material( context ) );
}

void Material::push( RenderContext & context, Color color, bool emissive, Blending blend )
{
	context.push( Material( color, emissive, blend ).material( context ) );
}

void Material::push( RenderContext & context, Blending blend )
{
	context.push( Material( blend ).material( context ) );
}

ColorFormat::Format Material::findBestFormat( DisplayDevice::Ref pDisplay, Image * pImage, bool bAllowCompressed )
{
	if (! pImage )
		return ColorFormat::INVALID;

	ColorFormat::Format eBestFormat = ColorFormat::INVALID;
	int bestScore = -1;

	for(int i=0;i<pDisplay->surfaceFormatCount();++i)
	{
		ColorFormat::Format eFormat = pDisplay->surfaceFormat(i);
		if ( !bAllowCompressed && (eFormat == ColorFormat::DXT1 || eFormat == ColorFormat::DXT3 || eFormat == ColorFormat::DXT5) )
			continue;				// no compressed format, skip..
		if ( eFormat == pImage->format() )
			return eFormat;			// found an exact match, always use those..

		int score = 0;
		switch( eFormat )
		{
		case ColorFormat::RGB8888:
			score = 9;
			break;
		case ColorFormat::RGB888:
		case ColorFormat::RGB888E:
			score = 8;
			break;
		case ColorFormat::RGB565:
		case ColorFormat::RGB565E:
			score = 7;
			break;
		case ColorFormat::RGB555:
			score = 6;
			break;
		case ColorFormat::RGB5551:
			score = 5;
			break;
		case ColorFormat::RGB4444:
			score = 1;
			break;
		case ColorFormat::RGB332:
			score = 0;
			break;
		}

		if ( score > bestScore || eBestFormat == ColorFormat::INVALID )
		{
			bestScore = score;
			eBestFormat = eFormat;
		}
	}

	return eBestFormat;
}


const WidgetKey MIPMAP_KEY( (qword)LL(4750620958941334377) );

PrimitiveSurface * Material::getSurface( DisplayDevice * pDisplay, Image::Link pImage, int nFrame, bool bMipMap )
{
	// generate a unique key from the components
	WidgetKey nKey = pImage.key() + nFrame;
	if ( bMipMap )
		nKey += MIPMAP_KEY;

	AutoLock lock( &sm_SurfaceHashLock );
	SurfaceHash::Iterator it = sm_SurfaceHash.find( nKey );
	if ( it.valid() )
		return *it;
	lock.release();

	// Disk texture cache -- fast path for second+ run (avoids Image format conversion / mip generation)
	CharString sCachePath = s_BuildTexCachePath( nKey );
	{
		PrimitiveSurface::Ref pCached;
		if ( s_LoadTexCache( sCachePath, pDisplay, pCached, bMipMap ) )
		{
			lock.set( &sm_SurfaceHashLock );
			sm_SurfaceHash[ nKey ] = pCached;
			return pCached;
		}
	}

	// surface not found, create and cache the surface
	PrimitiveSurface::Ref pSurface;
	// Use non-blocking valid() -- if the image is not yet loaded by Broker, return NULL and retry next frame.
	if ( pImage.valid( true, false ) && pImage->frameCount() > 0 )
	{
		// find a format for our surface first, findBestFormat() will always try to use the images current format
		// if supported by the hardware.
		ColorFormat::Format eFormat = findBestFormat( pDisplay, pImage, true );
		if ( eFormat == ColorFormat::INVALID )
		{
			TRACE( "ERROR: Failed to find a suitable surface format for texture!" );
			return NULL;
		}

		SizeInt imageSize( pImage->size() );
		SizeInt maxSize( pDisplay->textureMaxSize() );
		SizeInt minSize( pDisplay->textureMinSize() );

		// use the smaller max size
		maxSize.width = Min( sm_MaxTextureSize.width, maxSize.width );
		maxSize.height = Min( sm_MaxTextureSize.height, maxSize.height );

		// validate the texture dimensions
		SizeInt validSize( Max( Min( imageSize.width, maxSize.width ), minSize.width), 
			Max( Min( imageSize.height, maxSize.height ), minSize.height) );
		if ( pDisplay->textureP2() && !pImage->isP2() )
			validSize = SizeInt( 1 << GetLastBit( validSize.width ), 1 << GetLastBit( validSize.height) );
		if ( pDisplay->textureSquare() && validSize.width != validSize.height )
			validSize = SizeInt( Max( validSize.width, validSize.height ), Max( validSize.width, validSize.height) );

		if ( validSize != imageSize || eFormat != pImage->format() )
		{
			// image is the wrong size or format, make a copy so we don't modify the original..
			pImage = new Image( *pImage );

			if ( pImage->size() != validSize )
			{
				// image has to be resized - switch the format to an uncompressed format, DXT compression is too slow
				// to do on the fly..
				eFormat = findBestFormat( pDisplay, pImage, false );
				if ( eFormat != pImage->format() )
				{
					TRACE( CharString().format( "Image: Reformat %s -> %s", ColorFormat::formatText( pImage->format() ),
						ColorFormat::formatText( eFormat ) ) );
					if (! pImage->setFormat( eFormat ) )
						return NULL;
				}

				TRACE( CharString().format( "Image: Resize %dx%d -> %dx%d", imageSize.width, imageSize.height, 
					validSize.width, validSize.height ) );

				pImage->resample( validSize );
				if ( bMipMap && pImage->mipMap() != NULL )
					pImage->createMipMaps();
			}
			else if ( pImage->format() != eFormat )
			{
				TRACE( CharString().format( "Image: Reformat %s -> %s", ColorFormat::formatText( pImage->format() ),
					ColorFormat::formatText( eFormat ) ) );
				if (! pImage->setFormat( eFormat ) )
					return NULL;		// failed to convert to the desired format..
			}
		}

		if ( bMipMap && pImage->mipMap() == NULL )
			bMipMap = false;		// no mipmap levels in image, disable mipmaps

		// create the primitives, note having neither a diffuse or alpha texture is also a valid material
		pDisplay->create( pSurface );

		// create the surface
		SizeInt surfaceSize( pImage->size() );
		if ( !pSurface->initialize( 
			surfaceSize.width,
			surfaceSize.height, 
			eFormat,
			bMipMap ) )
		{
			TRACE( CharString().format("ERROR: Failed to initialize surface %dx%d format %d!", 
				surfaceSize.width, surfaceSize.height, eFormat) );
			pSurface = NULL;
		}

		nFrame = nFrame % pImage->frameCount();

		if ( pSurface.valid() )
		{
			Image * pMipMap = pImage;
			for(int i=0;i<pSurface->levels();++i)
			{
				if (! pMipMap )
				{
					// Image mip chain shorter than surface level count -- expected for
					// auto-mipmap surfaces; GPU fills remaining levels via GenerateMips.
					break;
				}

				void * pPixels = pSurface->lock( i );
				if (! pPixels )
				{
					// lock() returns NULL for level > 0 on auto-mipmap (non-BC) surfaces;
					// GPU generates those levels via GenerateMips after level 0 is uploaded.
					break;
				}

				// D3D11 staging textures may have RowPitch > width * bpp/8 due to GPU alignment.
				// Copy row-by-row so each source row lands at the correct destination offset.
				// BC (block-compressed) formats use 4x4-pixel blocks; rows are block-rows.
				const bool bBC = (eFormat == ColorFormat::DXT1 || eFormat == ColorFormat::DXT3 || eFormat == ColorFormat::DXT5);
				const int mipHeight  = pMipMap->size().height;
				const int numRows    = bBC ? Max(1, (mipHeight + 3) / 4) : mipHeight;
				const dword srcFrameSize = pMipMap->frameSize( nFrame );
				const int srcPitch   = (numRows > 0) ? (int)(srcFrameSize / numRows) : (int)srcFrameSize;
				const int dstPitch   = pSurface->pitch();

				if ( srcPitch == dstPitch )
				{
					memcpy( pPixels, pMipMap->frame( nFrame ), srcFrameSize );
				}
				else
				{
					TRACE( CharString().format("getSurface: pitch mismatch level=%d srcPitch=%d dstPitch=%d numRows=%d dstTotal=%d",
						i, srcPitch, dstPitch, numRows, dstPitch * numRows) );
					const byte* pSrc = (const byte*)pMipMap->frame( nFrame );
					byte* pDst = (byte*)pPixels;
					for ( int row = 0; row < numRows; ++row )
					{
						memcpy( pDst, pSrc, srcPitch );
						pSrc += srcPitch;
						pDst += dstPitch;
					}
				}

				pSurface->unlock();

				// next mipmap level of our image..
						pMipMap = pMipMap->mipMap();
					}

						// Queue the cache write so it runs off the render-frame hot path
						s_QueueSaveTexCache( sCachePath, pImage, nFrame, eFormat, pSurface->levels() );
					}
				}

	// only cache when the surface was successfully created; avoids permanently caching NULL
	// for images that are still loading asynchronously (they will retry on next getSurface call)
	if ( pSurface.valid() )
	{
		lock.set( &sm_SurfaceHashLock );
		sm_SurfaceHash[ nKey ] = pSurface;
	}
	return pSurface;
}

void Material::flushSurfaceCache()
{
	{
		AutoLock lock( &sm_SurfaceHashLock );
		sm_SurfaceHash.release();
	}
	// Also discard any materials still waiting for their first upload
	{
		AutoLock lock( &sm_PendingLock );
		sm_PendingMaterials.release();
	}
}

void Material::flushPendingDeviceSurfaces( DisplayDevice * pDevice )
{
	if ( !pDevice )
		return;

	// Drain pending .txc cache writes — 2 per frame to avoid burst disk I/O
	s_DrainSaveQueue();

	static const int MAX_PER_FRAME = 8;

	Array<Ref> toProcess;
	Array<Ref> requeue;
	{
		AutoLock lock( &sm_PendingLock );
		for ( int i = 0; i < sm_PendingMaterials.size(); ++i )
		{
			if ( !sm_PendingMaterials[i].valid() || sm_PendingMaterials[i]->m_bSurfaceReady )
				continue;
			if ( toProcess.size() < MAX_PER_FRAME )
				toProcess.push( sm_PendingMaterials[i] );
			else
				requeue.push( sm_PendingMaterials[i] );
		}
		sm_PendingMaterials.release();
		sm_PendingMaterials = requeue;
	}

	for ( int i = 0; i < toProcess.size(); ++i )
	{
		toProcess[i]->createDeviceSurfaces( pDevice );
		if ( !toProcess[i]->m_bSurfaceReady )
		{
			AutoLock lock( &sm_PendingLock );
			sm_PendingMaterials.push( toProcess[i] );
		}
	}
}

//-------------------------------------------------------------------------------

void Material::initialize()
{
	m_Flammable = 0.5f;
	m_Hardness = 0.5f;
	m_Strength = 0.5f;
	m_Friction = 0.5f;
	m_nPass = 1;
	m_Width = 10.0f;
	m_Height = 10.0f;

	m_Diffuse = Color(128,128,128,255);			// lighting properties
	m_Ambient = Color(128,128,128);
	m_Emissive = Color(0,0,0);
	m_Specular = Color(255,255,255);
	m_SpecularPower = 0.0f;
	m_LightEnable = true;

	m_Fps = 15.0f;
	m_Frames = 1;
	m_Blending = PrimitiveMaterial::NONE;
	m_DoubleSided = false;
	m_MipMap = false;							// MipMap enable flag

	m_fLastAlpha = 1.0f;
	m_nLastFrame = -1;
	m_bSurfaceReady = false;
}

void Material::createDevicePrimitives( RenderContext & context )
{
	DisplayDevice * pDisplay = context.display();
	ASSERT( pDisplay );

	m_Material = NULL;		// clear any previous primitive

	pDisplay->create( m_Material );
	if ( context.alpha() < 1.0f )
	{
		m_Material->setMaterial( Color( m_Diffuse.r, m_Diffuse.g, m_Diffuse.b, (u8)(context.alpha() * m_Diffuse.a) ), 
			m_Ambient, m_Emissive, m_Specular, m_SpecularPower );
		m_Material->setPass( DisplayDevice::SECONDARY );

		if ( m_Blending == PrimitiveMaterial::NONE )
			m_Material->setBlending( PrimitiveMaterial::ALPHA );
		else
			m_Material->setBlending( m_Blending );
	}
	else
	{
		m_Material->setMaterial( m_Diffuse, m_Ambient, m_Emissive, m_Specular, m_SpecularPower );
		m_Material->setPass( m_nPass );
		m_Material->setBlending( m_Blending );
	}

	m_Material->setLightEnable( m_LightEnable );
	m_Material->setDoubleSided( m_DoubleSided );
	if ( m_sShader.length() > 0 )
		m_Material->setShader( m_sShader );

	if (! m_bSurfaceReady )
		createDeviceSurfaces( context.display() );

	// attach the surfaces to the material now
	if ( m_Surfaces.size() > 0 )
	{
		Array< PrimitiveSurface::Ref > & surfaces = m_Surfaces[ 0 ];
		ASSERT( surfaces.size() == m_Textures.size() );

		for(int i=0;i<surfaces.size();++i)
		{
			if ( !surfaces[i].valid() )
				continue;
			Texture & texture = m_Textures[ i ];
			m_Material->addSurface( surfaces[i], 
				texture.m_eType,
				texture.m_nIndex, 
				texture.m_nUV, 
				texture.m_fParams );
		}
	}

	m_nLastFrame = 0;
	m_fLastAlpha = context.alpha();
}

void Material::createDeviceSurfaces( DisplayDevice * pDevice )
{
	// create the primitives, note having no textures is valid..
	bool bAllReady = true;
	if ( m_Textures.size() > 0 )
	{
		m_Surfaces.allocate( m_Frames );
		for(int f=0;f<m_Frames;++f)
		{
			Array< PrimitiveSurface::Ref > & surfaces = m_Surfaces[f];
			surfaces.allocate( m_Textures.size() );

			for(int t=0;t<m_Textures.size();++t)
			{
				Texture & texture = m_Textures[t];
				surfaces[t] = getSurface( pDevice, texture.m_pImage, f, texture.m_bMipMap );
				// If the surface is NULL, mark not-ready whenever the image key is non-null.
				// Covers: (a) image not yet loaded by Broker -- valid(true,false) returns false,
				// so the old guard missed this case and set m_bSurfaceReady=true prematurely;
				// (b) image loaded but surface creation failed -- retry next frame.
				if ( !surfaces[t].valid() && texture.m_pImage.key() != WidgetKey() )
					bAllReady = false;
			}
		}
	}
	m_bSurfaceReady = bAllReady;
}

//----------------------------------------------------------------------------
// EOF
