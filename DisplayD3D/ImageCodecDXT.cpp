/**
	@file ImageCodexDXT.cpp

	(c)2009 Palestar Inc
	@author Richard Lyle @date 9/3/2010 1:38:43 PM
*/

#include "ImageCodecDXT.h"

//! Define to non-zero to use DirectX surfaces for compression, then fallback to squish if needed..
//! NOTE: DirectX surface compression disabled for D3D11 - using Squish library instead
#define ENABLE_DX_SURFACE_COMPRESSION				0

// Squish Library include
#include "squish.h"

//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

static void SwapRedBlue( const Buffer & input, Buffer & output )
{
	dword nPixels = input.bufferSize() / sizeof(Color);
	output.allocate( input.bufferSize() );

	Color * pIn = (Color *)input.buffer();
	Color * pOut = (Color *)output.buffer();
	for(dword p=0;p<nPixels;++p)
		pOut[p] = pIn[p].BGRA();
}

//----------------------------------------------------------------------------

IMPLEMENT_FACTORY( ImageCodecDXT1, ImageCodec );

int	ImageCodecDXT1::encode( const Buffer & input, Buffer & output, const SizeInt & size, EncodeLevel nLevel )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( EncodeUsingDX( input, output, size, D3DFMT_DXT1 ) )
		return output.bufferSize();
#endif

	Buffer compress;
	SwapRedBlue( input, compress );

	output.allocate( GetStorageRequirements( size.width, size.height, squish::kDxt1 ) );
	squish::CompressImage( (squish::u8 *)compress.buffer(), size.width, size.height, output.buffer(), squish::kDxt1 );
	return output.bufferSize();
}

int ImageCodecDXT1::decode( const Buffer & input, Buffer & output, const SizeInt & size )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( DecodeUsingDX( input, output, size, D3DFMT_DXT1 ) )
		return output.bufferSize();
#endif

	Buffer decompressed;
	decompressed.allocate( size.width * size.height * sizeof(Color) );
	squish::DecompressImage( (squish::u8 *)decompressed.buffer(), size.width, size.height, input.buffer(), squish::kDxt1 );

	SwapRedBlue( decompressed, output );
	return output.bufferSize();
}

//----------------------------------------------------------------------------

IMPLEMENT_FACTORY( ImageCodecDXT3, ImageCodec );

int	ImageCodecDXT3::encode( const Buffer & input, Buffer & output, const SizeInt & size, EncodeLevel nLevel )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( EncodeUsingDX( input, output, size, D3DFMT_DXT3 ) )
		return output.bufferSize();
#endif

	Buffer compress;
	SwapRedBlue( input, compress );

	output.allocate( GetStorageRequirements( size.width, size.height, squish::kDxt3 ) );
	squish::CompressImage( (squish::u8 *)compress.buffer(), size.width, size.height, output.buffer(), squish::kDxt3 );
	return output.bufferSize();
}

int ImageCodecDXT3::decode( const Buffer & input, Buffer & output, const SizeInt & size )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( DecodeUsingDX( input, output, size, D3DFMT_DXT3 ) )
		return output.bufferSize();
#endif

	Buffer decompressed;
	decompressed.allocate( size.width * size.height * sizeof(Color) );
	squish::DecompressImage( (squish::u8 *)decompressed.buffer(), size.width, size.height, input.buffer(), squish::kDxt3 );

	SwapRedBlue( decompressed, output );
	return output.bufferSize();
}

//----------------------------------------------------------------------------

IMPLEMENT_FACTORY( ImageCodecDXT5, ImageCodec );

int	ImageCodecDXT5::encode( const Buffer & input, Buffer & output, const SizeInt & size, EncodeLevel nLevel )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( EncodeUsingDX( input, output, size, D3DFMT_DXT5 ) )
		return output.bufferSize();
#endif

	Buffer compress;
	SwapRedBlue( input, compress );

	output.allocate( GetStorageRequirements( size.width, size.height, squish::kDxt5 ) );
	squish::CompressImage( (squish::u8 *)compress.buffer(), size.width, size.height, output.buffer(), squish::kDxt5 );
	return output.bufferSize();
}

int ImageCodecDXT5::decode( const Buffer & input, Buffer & output, const SizeInt & size )
{
#if ENABLE_DX_SURFACE_COMPRESSION
	if ( DecodeUsingDX( input, output, size, D3DFMT_DXT5 ) )
		return output.bufferSize();
#endif

	Buffer decompressed;
	decompressed.allocate( size.width * size.height * sizeof(Color) );
	squish::DecompressImage( (squish::u8 *)decompressed.buffer(), size.width, size.height, input.buffer(), squish::kDxt5 );

	SwapRedBlue( decompressed, output );
	return output.bufferSize();
}

//---------------------------------------------------------------------------------------------------
//EOF
