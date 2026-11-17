#ifndef MEDUSA_PRIMITIVE_PLANET_ATMOSPHERE_H
#define MEDUSA_PRIMITIVE_PLANET_ATMOSPHERE_H

#include "Display/DevicePrimitive.h"
#include "Math/Vector3.h"
#include "MedusaDll.h"

class PrimitivePlanetAtmosphere : public DevicePrimitive
{
public:
	typedef Reference<PrimitivePlanetAtmosphere>Ref;

	static Reference<DevicePrimitive> create( DisplayDevice * pDisplay,
		float fRadius, const Vector3 & vColor = Vector3(1,1,1),
		float fDensity = 0.6f, float fAnimSpeed = 1.0f );

virtual void setRadius( float fRadius ) = 0;
virtual void setDensity( float fDensity ) = 0;
virtual void setColor( const Vector3 & vColor ) = 0;
virtual void setAnimSpeed( float fAnimSpeed ) = 0;

virtual float getRadius() const = 0;
virtual float getDensity() const = 0;
virtual Vector3 getColor() const = 0;
virtual float getAnimSpeed() const = 0;

protected:
	PrimitivePlanetAtmosphere() {}
	virtual ~PrimitivePlanetAtmosphere() {}
};

#endif