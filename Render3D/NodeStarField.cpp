/*
	NodeStarField.cpp
	(c)2005 Palestar Inc, Richard Lyle
*/

#define RENDER3D_DLL
#include "Debug/Assert.h"
#include "Math/Helpers.h"
#include "Render3D/NodeStarField.h"
#include "Display/PrimitiveSetTransform.h"

//----------------------------------------------------------------------------

Color NodeStarField::s_StarColor( 255, 255, 255, 255 );
float NodeStarField::s_StarSize = 0.001f;
float NodeStarField::s_DopplerVelocity = 500.0f;

const Color BLUE_SHIFT( 128,128,255,128 );
const Color RED_SHIFT( 255,128,128,128 );

//----------------------------------------------------------------------------

IMPLEMENT_FACTORY( NodeStarField, BaseNode );
REGISTER_FACTORY_KEY(  NodeStarField, 4400228791173987159LL );

BEGIN_PROPERTY_LIST( NodeStarField, BaseNode )
	ADD_PROPERTY( m_ParticleCount );
	ADD_PROPERTY( m_Front );
	ADD_PROPERTY( m_Back );
END_PROPERTY_LIST();

//----------------------------------------------------------------------------

NodeStarField::NodeStarField() : m_bActive( true ), m_bTrailActive( true ), m_bJumpActive( false )
{
	m_ParticleCount = 150;
	m_Front = 1.0f;
	m_Back = 500.0f;
	m_LastTime = 0.0f;
	m_LastFrame.identity();
}

//----------------------------------------------------------------------------

void NodeStarField::render( RenderContext &context, 
		const Matrix33 & frame, 
		const Vector3 & position )
{
	if (! m_bActive )
		return;

	DisplayDevice * pDisplay = context.display();
	ASSERT( pDisplay );

	// Apply full camera-motion correction (translation + rotation) to all existing stars
	// BEFORE any new stars are created, so freshly spawned stars are not double-corrected.
	//
	// The goal is a "fixed sky" — stars keep their apparent angular position regardless of
	// how the camera translates or rotates (e.g. auto-orbiting the spawn point).
	//
	// For each star we recover its view-space position under the previous frame's camera,
	// then re-express it in the current camera's frame:
	//   vsOld      = lastFrame    * (star.world - lastPos)       [old view-space offset]
	//   star.world = context.pos  + context.frame % vsOld        [back to world space in new frame]
	// operator*(Vector3) = TransToFrame  (world→view)
	// operator%(Vector3) = TransFromFrame (view→world, i.e. ~frame * v)
	{
		Matrix33 lastFrame  = m_LastFrame;
		Vector3  lastPos    = m_LastPosition;
		m_LastFrame    = context.frame();
		m_LastPosition = context.position();

		for(int i=0;i<m_Stars.size();i++)
		{
			Star & star = m_Stars[i];
			star.m_vHead = context.position() + (context.frame() % (lastFrame * (star.m_vHead - lastPos)));
			star.m_vTail = context.position() + (context.frame() % (lastFrame * (star.m_vTail - lastPos)));
		}
	}

	if ( m_Stars.size() != m_ParticleCount )
	{
		int nPreviousCount = m_Stars.size();
		m_Stars.realloc( m_ParticleCount );
		for(int i=nPreviousCount;i<m_ParticleCount;i++)
			createParticle( context, i );
	}

	const Vector3 N(0,0,-1.0f);

	// get the star color
	Color color( s_StarColor );

	// allocate a triangle list
	PrimitiveTriangleListDL::Ref pTriangleList = 
		PrimitiveTriangleListDL::create( pDisplay, m_ParticleCount );

	// update the triangle list
	VertexL * pVertex = (VertexL *)pTriangleList->lock();

	// Billboard axes in VIEW space.  pushIdentity() sets mWorld=I and mView=I so
	// vertex positions must be view-space; (1,0,0)/(0,1,0) are screen-aligned.
	const Vector3 vRight(1.0f, 0.0f, 0.0f);
	const Vector3 vUp   (0.0f, 1.0f, 0.0f);
	const Vector3 & camPos = context.position();

	for(int i=0;i<m_Stars.size();i++)
	{
		Star & star = m_Stars[ i ];

		color = s_StarColor;

		// Compute view-space depth for size/alpha/culling.
		// star.m_vHead is world-space; star.m_vTail is the previous frame's world-space head.
		Vector3 vsHead( context.worldToView( star.m_vHead ) );
		float fDepth = vsHead.z;		// save for size/alpha before any projection

		// Update tail to this frame's world-space position for the next frame.
		Vector3 wTailStar = star.m_vTail;
		star.m_vTail = star.m_vHead;

		// Submit vertices in world space. pushIdentity() sets mWorld=I and
		// mView = LookAtLH(camPos,...) transforms world->view on the GPU.
		// Do NOT subtract camPos here — mView already does that.
		Vector3 vHead = star.m_vHead;
		Vector3 vTail = wTailStar;

		if ( m_bJumpActive )
		{
			// Doppler: use original (un-projected) view depths.
			Vector3 vsTail( context.worldToView( wTailStar ) );
			float deltaZ = (fDepth - vsTail.z) / context.elapsed();
			if ( deltaZ < 0.0f )
				color.iterpolate( BLUE_SHIFT, Clamp<float>( -deltaZ / s_DopplerVelocity, 0.0f, 1.0f ) );
			else if ( deltaZ > 0.0f )
				color.iterpolate( RED_SHIFT, Clamp<float>( deltaZ / s_DopplerVelocity, 0.0f, 1.0f ) );
		}

		float size  = s_StarSize * fDepth;
		float alpha = 1.0f - (fDepth / m_Back);
		color.a = (u8)( 255.0f * alpha );

		// Vertices are in VIEW SPACE; pushIdentity() lets mProj project correctly.
		pVertex->position = vHead + vUp * size;
		pVertex->diffuse = color;
		pVertex->normal = N;
		pVertex->u = 0.0f;
		pVertex->v = 0.0f;
		pVertex++;

		pVertex->position = vHead + vRight * size;
		pVertex->diffuse = color;
		pVertex->normal = N;
		pVertex->u = 0.0f;
		pVertex->v = 0.0f;
		pVertex++;

		if ( m_bTrailActive || m_bJumpActive )
		{
			pVertex->position = vTail;
			pVertex->diffuse = BLACK;
			pVertex->normal = N;
			pVertex->u = 0.0f;
			pVertex->v = 0.0f;
			pVertex++;
		}
		else
		{
			pVertex->position = vHead - vRight * size;
			pVertex->diffuse = color;
			pVertex->normal = N;
			pVertex->u = 0.0f;
			pVertex->v = 0.0f;
			pVertex++;
		}

		// Cull using the view-space position (before far-plane projection).
		if ( fDepth <= 0.0f || fabs( vsHead.x ) > vsHead.z || fabs( vsHead.y ) > vsHead.z || vsHead.z > m_Back || vsHead.z < m_Front )
		{
			Vector3 vRandomDirection( RandomVector( Vector3( -1.0f, -1.0f, -1.0f ), Vector3( 1.0f, 1.0f, 1.0f ) ) );
			vRandomDirection.normalize();

			// Spawn throughout the full depth range so stars fill the entire volume
			// (front to back) rather than clustering in a shell at the outer half.
			star.m_vHead = camPos + (vRandomDirection * RandomFloat( m_Front, m_Back ));
			star.m_vTail = star.m_vHead;	// reset trail to same position (world space)
		}
	}

	pTriangleList->unlock();

	// push the primitives
	// Star vertices are in VIEW SPACE; pushIdentity() sets mWorld=I so
	// mProj projects them correctly as background sky.
	// lightEnable=false: VertexL stars are pre-lit — vertex alpha IS the brightness.
	context.push( PrimitiveMaterial::create( pDisplay, PrimitiveMaterial::ADDITIVE, false, false ) );
	context.pushIdentity();
	context.push( pTriangleList );

	BaseNode::render( context, frame, position );
}

//----------------------------------------------------------------------------

void NodeStarField::setActive( bool bActive )
{
	m_bActive = bActive;
}

void NodeStarField::setTrailActive( bool bTrailActive )
{
	m_bTrailActive = bTrailActive;
}

void NodeStarField::setJumpActive( bool bDopplerActive )
{
	m_bJumpActive = bDopplerActive;
}

void NodeStarField::initialize( int particles, float front, float back )
{
	m_bActive = true;
	m_ParticleCount = particles;
	m_Front = front;
	m_Back = back;
}

void NodeStarField::setParticleCount( int particles )
{
	m_ParticleCount = particles;
}

//----------------------------------------------------------------------------

void NodeStarField::createParticle( RenderContext & context, int n )
{
	// create particle somewhere in the viewspace of the context
	Vector3 vsp( RandomFloat( -0.5f, 0.5f ), RandomFloat( -0.5f, 0.5f ), 1.0f );
	vsp *= RandomFloat( m_Front, m_Back );

	// transform from viewspace to world space; store head and tail at the same world position
	Star & star = m_Stars[ n ];
	star.m_vTail = star.m_vHead = context.viewToWorld( vsp );
}

//----------------------------------------------------------------------------
// EOF
