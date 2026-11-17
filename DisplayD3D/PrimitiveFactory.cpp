/*
	PrimitiveFactory.cpp
	(c)2005 Palestar, Richard Lyle
*/
#include <Windows.h>
#include <mutex>
#include "Debug/Trace.h"
#include "Standard/Hash.h"
#include "Standard/StringHash64.h"
#include "PrimitiveFactory.h"
#include "PrimitiveMaterialD3D.h"
#include "PrimitiveWindowD3D.h"
#include "PrimitiveSurfaceD3D.h"
#include "ProceduralPlanetD3D.h"

//---------------------------------------------------------------------------------------

typedef Hash<PrimitiveKey,BasePrimitiveFactory *>		PrimitiveFactoryHash;

PrimitiveFactoryHash & primitiveFactoryHash()
{
	static PrimitiveFactoryHash s_PrimitiveFactoryHash;
	return( s_PrimitiveFactoryHash );
}

//---------------------------------------------------------------------------------------

// Forward declare the factory instances created by IMPLEMENT_PRIMITIVE_FACTORY macros
extern PrimitiveFactory<PrimitiveMaterialD3D> g_PrimitiveMaterialD3DFactory;
extern PrimitiveFactory<PrimitiveSurfaceD3D> g_PrimitiveSurfaceD3DFactory;
extern PrimitiveFactory<PrimitiveWindowD3D> g_PrimitiveWindowD3DFactory;
extern PrimitiveFactory<PrimitiveProceduralPlanetD3D> g_PrimitiveProceduralPlanetD3DFactory;

//---------------------------------------------------------------------------------------

BasePrimitiveFactory::BasePrimitiveFactory( const PrimitiveKey & key ) : m_Key(key)
{
	PrimitiveFactoryHash & hash = primitiveFactoryHash();
	if ( hash.find( key ).valid() )
		throw KeyConflict();

	hash[ key ] = this;
}

BasePrimitiveFactory::~BasePrimitiveFactory()
{
	primitiveFactoryHash().remove( m_Key );
}

//----------------------------------------------------------------------------

BasePrimitiveFactory * BasePrimitiveFactory::findFactory( const PrimitiveKey &key )
{
	PrimitiveFactoryHash & hash = primitiveFactoryHash();

	PrimitiveFactoryHash::Iterator iFactory = hash.find( key );
	if ( iFactory.valid() )
		return *iFactory;

	return NULL;
}

void BasePrimitiveFactory::initializeFactories()
{
	// Guard against multiple initializations - use static local for C++11 thread-safe init
	static std::once_flag s_initFlag;
	std::call_once(s_initFlag, []()
	{
		OutputDebugStringA("BasePrimitiveFactory::initializeFactories() - START\n");

		// Force the linker to include the D3D factory globals from their translation units.
		// Without these volatile references, the linker may strip the objects as unreferenced,
		// which prevents their constructors from auto-registering into the hash.
		volatile void* p1 = &g_PrimitiveMaterialD3DFactory;
		volatile void* p2 = &g_PrimitiveSurfaceD3DFactory;
		volatile void* p3 = &g_PrimitiveWindowD3DFactory;
		volatile void* p4 = &g_PrimitiveProceduralPlanetD3DFactory;
		(void)p1; (void)p2; (void)p3; (void)p4;

		PrimitiveFactoryHash & hash = primitiveFactoryHash();

		PrimitiveKey materialKey(StringHash64("PrimitiveMaterial"));
			PrimitiveKey surfaceKey(StringHash64("PrimitiveSurface"));
			PrimitiveKey windowKey(StringHash64("PrimitiveWindow"));
			PrimitiveKey procPlanetKey(StringHash64("PrimitiveProceduralPlanet"));

			hash[ materialKey ] = &g_PrimitiveMaterialD3DFactory;
			hash[ surfaceKey ] = &g_PrimitiveSurfaceD3DFactory;
			hash[ windowKey ] = &g_PrimitiveWindowD3DFactory;
			hash[ procPlanetKey ] = &g_PrimitiveProceduralPlanetD3DFactory;

			char buf[512];
			sprintf_s(buf, "Registered base class keys:\n  PrimitiveMaterial: %I64u\n  PrimitiveSurface: %I64u\n  PrimitiveWindow: %I64u\n  PrimitiveProceduralPlanet: %I64u\n  Total entries: %d\n",
				((UniqueNumber&)materialKey).m_Id,
				((UniqueNumber&)surfaceKey).m_Id, 
				((UniqueNumber&)windowKey).m_Id,
				((UniqueNumber&)procPlanetKey).m_Id,
				hash.size());
					OutputDebugStringA(buf);
				});
			}

			//---------------------------------------------------------------------------------------
			// EOF
