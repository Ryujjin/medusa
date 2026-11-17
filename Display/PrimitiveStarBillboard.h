/*
PrimitiveStarBillboard.h
Abstract device primitive for rendering the extended stellar corona and CME
filaments as a flat billboard quad using VS_Star / PS_Star.

The sphere (PrimitiveStarSurface) renders the photosphere + chromosphere on the
disc (r <= 1).  This billboard covers the full ±2.5 disk-radii region and lets
PS_Star render the K-corona halo, spike arms, solar flares and CME filaments
that extend beyond the geometric silhouette of the sphere.

Both primitives should be submitted ADDITIVE with pushIdentity so that they
composite correctly in the additive star-field pass.

(c)2025 DarkSpace DX11 Port
*/

#ifndef PRIMITIVE_STAR_BILLBOARD_H
#define PRIMITIVE_STAR_BILLBOARD_H

#include "Math/Vector3.h"
#include "Display/DisplayDevice.h"
#include "MedusaDll.h"

//----------------------------------------------------------------------------

class DLL PrimitiveStarBillboard : public DevicePrimitive
{
public:
    DECLARE_DEVICE_PRIMITIVE();

    typedef Reference<PrimitiveStarBillboard>  Ref;

    // Shared vertex layout: pos(12) + normal(12) + color(4) + uv(8) = 36 bytes
    // Matches VertexL / m_pInputLayoutL in D3D layer.
    struct Vertex {
        float    px, py, pz;
        float    nx, ny, nz;
        unsigned diffuse;
        float    u,  v;
    };

    // center    : world-space billboard center (camPos + starDir * m_fBack)
    // right     : world-space right vector (camera right, normalized)
    // up        : world-space up vector (camera up, normalized)
    // halfSize  : half-size of the quad in world units (renderRadius after 2.5x expansion)
    // starColor : RGB spectral colour forwarded to vStarColor in CB0
    virtual bool initialize( const Vector3 & center,
                              const Vector3 & right,
                              const Vector3 & up,
                              float           halfSize,
                              const Vector3 & starColor ) = 0;

    // Atmosphere sphere mesh — arbitrary triangle list in VertexL layout.
    // Caller builds camera-projected UVs so PS_Star disk-space math is correct.
    virtual bool initializeMesh( const Vertex * pVerts,
                                 int            nVerts,
                                 const Vector3 & starColor ) = 0;
};

// Free-function factory — avoids dllimport/inline conflict (MSVC Session 72 rule).
inline PrimitiveStarBillboard::Ref createPrimitiveStarBillboard(
    DisplayDevice *  pDevice,
    const Vector3 &  center,
    const Vector3 &  right,
    const Vector3 &  up,
    float            halfSize,
    const Vector3 &  starColor )
{
    PrimitiveStarBillboard::Ref pPrimitive;
    pDevice->create( pPrimitive );
    pPrimitive->initialize( center, right, up, halfSize, starColor );
    return pPrimitive;
}

//----------------------------------------------------------------------------

#endif // PRIMITIVE_STAR_BILLBOARD_H

// EOF
