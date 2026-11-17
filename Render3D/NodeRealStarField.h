/*
	NodeRealStarField.h
	Background star field rendered from real star positions (Yale BSC5P subset).

	Key differences from NodeStarField:
	  - Stars are at FIXED DIRECTIONS in an inertial celestial frame.
	    They only move as the camera ROTATES, not as it translates.
	    There is no "flying through the stars" parallax — real stars are
	    effectively at infinite distance.
	  - Star colours are derived from spectral type (O/B=blue, A=white,
	    F=yellow-white, G=yellow, K=orange, M=red).
	  - Brightness is proportional to apparent visual magnitude.
	  - Named stars can be looked up by direction for HUD labels.

	Coordinate mapping:
	  The catalog stores J2000 equatorial unit vectors.
	  s_CelestialFrame maps equatorial axes to game-world axes.
	  Default mapping:
	    game +X = celestial +x  (towards RA=0h, Dec=0)
	    game +Y = celestial +z  (towards north celestial pole)
	    game +Z = celestial +y  (towards RA=6h, Dec=0)

	  Call NodeRealStarField::setCelestialFrame() once at startup
	  to re-orient the sky if the game's world "up" differs.
*/

#ifndef NODEREALSTARFIELD_H
#define NODEREALSTARFIELD_H

#include "Render3D/BaseNode.h"
#include "Display/PrimitiveTriangleList.h"
#include "Display/PrimitiveGalaxy.h"
#include "Display/PrimitiveGalaxySpriteSet.h"
#include "Display/PrimitiveStarSurface.h"
#include "Render3D/Render3dDll.h"

//----------------------------------------------------------------------------

class DLL NodeRealStarField
{
public:
	// Construction
	NodeRealStarField();

	// Interface compatible with NodeStarField usage in GameContext
	void	render( RenderContext & context,
					const Matrix33 & frame,
					const Vector3 & position );

	void	setActive( bool bActive )    { m_bActive = bActive; }
	bool	active()              const  { return m_bActive; }

	// initialize() is a no-op for real stars; depth range only affects the
	// radius at which star quads are placed (no parallax effect)
	void	initialize( int /*particles*/, float /*front*/, float back )
			{ m_fBack = back; }

	// setParticleCount() is a no-op for real stars (catalog size is fixed),
	// but kept for API compatibility with GadgetJumpDrive / other callers.
	void	setParticleCount( int /*n*/ ) {}

	// Jump-drive state kept for API compatibility; real stars have no Doppler.
	void	setTrailActive( bool b )  { m_bTrailActive = b; }
	void	setJumpActive(  bool b )  { m_bJumpActive  = b; }
	bool	isTrailActive()  const    { return m_bTrailActive; }
	bool	isJumpActive()   const    { return m_bJumpActive; }

	// Star-size scale factor (applied to magnitude-derived angular size)
	static float	s_StarSize;

	// Look up the name of the nearest catalog star to a given world-space
	// direction from the camera. Returns NULL if no named star is close.
	// dot_threshold: cos(angle) — e.g. 0.9998 ≈ ±1.1° tolerance.
	const char *	nearestName( const Vector3 & worldDir,
								 float dot_threshold = 0.9998f ) const;

	// Replace the default celestial→game mapping. Call once at startup
	// to orient the sky. Row-major: transforms a celestial unit vector to
	// the game's world-space: gameVec = celVec * s_CelestialFrame
	static void		setCelestialFrame( const Matrix33 & m ) { s_CelestialFrame = m; }

	// Notify the star field which real star system the camera is currently in.
	// The named star is excluded from the rendered catalog (you are orbiting it,
	// so it should not appear as a background point).  Pass NULL to restore the
	// full catalog.  Has no effect if the name is unchanged (no rebuild cost).
	// Call whenever the active zone changes (e.g. map load, system jump).
	void			setLocalSystem( const char * starName );

	// Pass the actual world-space position of the local star (NounStar::worldPosition()).
	// render() uses this each frame to compute the exact camera-relative direction so the
	// PS_Star billboard tracks the scene sphere precisely as the camera orbits.
	void			setLocalStarWorldPos( const Vector3 & worldPos ) { m_LocalStarWorldPos = worldPos; m_bLocalWorldPosSet = true; }

	// Pass the physical radius of the local star (NounStar::radius()) so render() can
	// scale the PS_Star billboard to maintain constant angular size as the camera moves.
	void			setLocalStarRadius( float radius ) { m_fLocalStarRadius = radius; }

private:
	// Types
	struct CatalogStar
	{
		Vector3     dir;          // unit vector in game-world space
		Color       color;        // RGBA vertex color (pre-computed)
		float       size;         // angular size scaled from magnitude
		float       flickerPhase; // per-star phase offset for twinkle animation (0..1)
		const char* name;         // pointer into g_StarCatalog (may be NULL)
	};

	struct GalaxyPatch
	{
		Vector3 dir;      // unit direction on galactic great circle (patch centre)
		Vector3 tangent;  // unit tangent along the circle (longitude axis)
		float   alpha;    // per-patch longitude brightness scale [0.12, 0.64]
		float   halfH;    // half-height in world units (wider near galCen for bulge)
	};

	// Per-entry for survey galaxy billboard sprites
	struct GalaxySpriteStar
	{
		Vector3 dir;       // unit direction in game-world space
		float   size;      // half-size of billboard (world units)
		float   u0, v0;    // atlas UV top-left
		float   u1, v1;    // atlas UV bottom-right
		float   brightness; // pre-computed brightness [0,1] from apparent magnitude
	};

	// Data
	bool            m_bActive;
	bool            m_bTrailActive;
	bool            m_bJumpActive;
	float           m_fBack;   // radius of star sphere (units)

	// Stars prepared from catalog (built once on first render, rebuilt on zone change)
	bool            m_bBuilt;
	Array<CatalogStar>       m_Stars;
	Array<GalaxyPatch>       m_GalaxyPatches;    // Milky Way band patches (built with catalog)
	Array<GalaxySpriteStar>  m_GalaxySprites;    // Survey galaxy billboard sprites
	Vector3             m_GnpDir;           // galactic north pole in game-world space
	Vector3             m_GalCenDir;        // galactic centre direction (equatorial, orthogonal to GNP)

	// Local star exclusion
	char            m_LocalStarName[64];  // catalog name to skip ('\0' = none)
	bool            m_bLocalSet;          // true when a local star name is set
	Vector3         m_StarColor;          // spectral RGB (0..1) for local system star
	Vector3         m_LocalStarDir;       // world-space direction to local star (catalog fallback)
	Vector3         m_LocalStarWorldPos;  // actual world-space position of the local NounStar
	bool            m_bLocalWorldPosSet;  // true when m_LocalStarWorldPos has been assigned
	float           m_fLocalStarRadius;   // physical radius of the local NounStar (game units)

	// Static
	static Matrix33 s_CelestialFrame;  // equatorial -> game-world transform

	// Helpers
	void            buildCatalog();
};

//----------------------------------------------------------------------------

#endif // NODEREALSTARFIELD_H
