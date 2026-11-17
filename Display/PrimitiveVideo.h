/*
	PrimitiveVideo.h
	Abstract device primitive for fullscreen video / intro-screen playback via
	the DX11 render pipeline.  A decoded video frame is uploaded to t8 (txVideo)
	each time the caller calls updateFrame(); the primitive then draws a fullscreen
	covering triangle with VS_Video + PS_Video.

	Usage:
		PrimitiveVideo::Ref pVid = PrimitiveVideo::create( pDevice );
		pVid->initialize( width, height );
		// each frame:
		pVid->updateFrame( pRGBAData, rowPitch );
		pVid->setAlpha( fadeValue );        // 0.0f..1.0f
		pVid->setLetterbox( true );         // preserve 16:9 aspect
		pDevice->push( pVid );              // submits to primitive queue

	The primitive can be pushed from any render context: intro scenes,
	in-game cut-scenes, or the HUD overlay pipeline.

	(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_VIDEO_H
#define PRIMITIVE_VIDEO_H

#include "Display/DisplayDevice.h"
#include "MedusaDll.h"

//----------------------------------------------------------------------------

class DLL PrimitiveVideo : public DevicePrimitive
{
public:
	DECLARE_DEVICE_PRIMITIVE();

	typedef Reference<PrimitiveVideo> Ref;

	// Initialize the GPU texture for video frames of the given pixel dimensions.
	// Must be called once before the first updateFrame().
	virtual bool	initialize( int width, int height ) = 0;

	// Upload a new RGBA video frame from CPU memory.
	//   pData     : pointer to pixel data, top-row first, RGBA8 (4 bytes/pixel)
	//   rowPitch  : byte stride of one row (== width*4 for tightly packed data)
	virtual void	updateFrame( const void* pData, int rowPitch ) = 0;

	// Set overall opacity (0 = transparent, 1 = fully opaque).  Default 1.0f.
	virtual void	setAlpha( float alpha ) = 0;

	// When true the shader adds pillar/letter-box black bars to preserve 16:9.
	virtual void	setLetterbox( bool enable ) = 0;

	// Convenience factory: creates + initialises in one call.
	// Returns a null Ref (check .valid()) if no factory is registered for PrimitiveVideo.
	static Ref		create( DisplayDevice* pDevice, int width, int height )
	{
		try
		{
			Ref pPrimitive;
			pDevice->create( pPrimitive );
			pPrimitive->initialize( width, height );
			return pPrimitive;
		}
		catch( DisplayDevice::PrimitiveFailure & )
		{
			return Ref();
		}
	}
};

//----------------------------------------------------------------------------

#endif

//----------------------------------------------------------------------------
// EOF
