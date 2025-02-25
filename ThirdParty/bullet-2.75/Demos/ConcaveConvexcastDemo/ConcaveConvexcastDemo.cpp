/*
Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2006 Erwin Coumans  http://continuousphysics.com/Bullet/

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, 
including commercial applications, and to alter it and redistribute it freely, 
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "btBulletDynamicsCommon.h"
#include "LinearMath/btIDebugDraw.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "GLDebugDrawer.h"
#include "ConcaveConvexcastDemo.h"
#include "GL_ShapeDrawer.h"
#include "GlutStuff.h"

#define NUM_DYNAMIC_BOXES_X 30
#define NUM_DYNAMIC_BOXES_Y 30

static btVector3*	gVertices=0;
static int*	gIndices=0;
static btBvhTriangleMeshShape* trimeshShape =0;
static btRigidBody* staticBody = 0;
static float waveheight = 5.f;

const float TRIANGLE_SIZE=8.f;


/* Scrolls back and forth over terrain */
#define NUMRAYS_IN_BAR 100
class btConvexcastBatch
{
public:
	btVector3 source[NUMRAYS_IN_BAR];
	btVector3 dest[NUMRAYS_IN_BAR];
	btVector3 direction[NUMRAYS_IN_BAR];
	btVector3 hit_com[NUMRAYS_IN_BAR];
	btVector3 hit_surface[NUMRAYS_IN_BAR];
	btScalar hit_fraction[NUMRAYS_IN_BAR];
	btVector3 normal[NUMRAYS_IN_BAR];

	int frame_counter;
	int ms;
	int sum_ms;
	int sum_ms_samples;
	int min_ms;
	int max_ms;

#ifdef USE_BT_CLOCK
	btClock frame_timer;
#endif //USE_BT_CLOCK

	btScalar dx;
	btScalar min_x;
	btScalar max_x;
	btScalar min_y;
	btScalar max_y;
	btScalar sign;

	btVector3  boxShapeHalfExtents;
	btBoxShape boxShape;

	btConvexcastBatch () : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
	}

	btConvexcastBatch (bool unused, btScalar ray_length, btScalar min_z, btScalar max_z, btScalar min_y , btScalar max_y ) : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		boxShapeHalfExtents = btVector3(1.0, 1.0, 1.0);
		boxShape = btBoxShape(boxShapeHalfExtents);
		frame_counter = 0;
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
		dx = 10.0;
		min_x = -40;
		max_x = 20;
		this->min_y = min_y;
		this->max_y = max_y;
		sign = 1.0;
		btScalar dalpha = 2*SIMD_2_PI/NUMRAYS_IN_BAR;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btScalar z = (max_z-min_z)/NUMRAYS_IN_BAR * i + min_z;
			source[i] = btVector3(min_x, max_y, z);
			dest[i] = btVector3(min_x + ray_length, min_y, z);
			normal[i] = btVector3(1.0, 0.0, 0.0);
		}
	}

	btConvexcastBatch (btScalar ray_length, btScalar z, btScalar min_y = -1000, btScalar max_y = 10) : boxShape(btVector3(0.0, 0.0, 0.0))
	{
		boxShapeHalfExtents = btVector3(1.0, 1.0, 1.0);
		boxShape = btBoxShape(boxShapeHalfExtents);
		frame_counter = 0;
		ms = 0;
		max_ms = 0;
		min_ms = 9999.0;
		sum_ms_samples = 0;
		sum_ms = 0;
		dx = 10.0;
		min_x = -40;
		max_x = 20;
		this->min_y = min_y;
		this->max_y = max_y;
		sign = 1.0;
		btScalar dalpha = 2*SIMD_2_PI/NUMRAYS_IN_BAR;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btScalar alpha = dalpha * i;
			// rotate around by alpha degrees y 
			btTransform tr(btQuaternion(btVector3(0.0, 1.0, 0.0), alpha));
			direction[i] = btVector3(1.0, 0.0, 0.0);
			direction[i] = tr * direction[i];
			direction[i] = direction[i];
			source[i] = btVector3(min_x, max_y, z);
			dest[i] = source[i] + direction[i] * ray_length;
			dest[i][1] = min_y;
			normal[i] = btVector3(1.0, 0.0, 0.0);
		}
	}

	void move (btScalar dt)
	{
		if (dt > (1.0/60.0))
			dt = 1.0/60.0;
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			source[i][0] += dx * dt * sign;
			dest[i][0] += dx * dt * sign;
		}
		if (source[0][0] < min_x)
			sign = 1.0;
		else if (source[0][0] > max_x)
			sign = -1.0;
	}

	void cast (btCollisionWorld* cw)
	{
#ifdef USE_BT_CLOCK
		frame_timer.reset ();
#endif //USE_BT_CLOCK
		for (int i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btCollisionWorld::ClosestConvexResultCallback cb(source[i], dest[i]);
			btQuaternion qFrom;
			btQuaternion qTo;
			qFrom.setRotation (btVector3(1.0, 0.0, 0.0), 0.0);
			qTo.setRotation (btVector3(1.0, 0.0, 0.0), 0.7);
			btTransform from(qFrom, source[i]);
			btTransform to(qTo, dest[i]);
			cw->convexSweepTest (&boxShape, from, to, cb);
			if (cb.hasHit ())
			{
				hit_surface[i] = cb.m_hitPointWorld;
				hit_com[i].setInterpolate3(source[i], dest[i], cb.m_closestHitFraction);
				hit_fraction[i] = cb.m_closestHitFraction;
				normal[i] = cb.m_hitNormalWorld;
				normal[i].normalize ();
			} else {
				hit_com[i] = dest[i];
				hit_surface[i] = dest[i];
				hit_fraction[i] = 1.0f;
				normal[i] = btVector3(1.0, 0.0, 0.0);
			}

		}
#ifdef USE_BT_CLOCK
		ms += frame_timer.getTimeMilliseconds ();
#endif //USE_BT_CLOCK
		frame_counter++;
		if (frame_counter > 50)
		{
			min_ms = ms < min_ms ? ms : min_ms;
			max_ms = ms > max_ms ? ms : max_ms;
			sum_ms += ms;
			sum_ms_samples++;
			btScalar mean_ms = (btScalar)sum_ms/(btScalar)sum_ms_samples;
			printf("%d rays in %d ms %d %d %f\n", NUMRAYS_IN_BAR * frame_counter, ms, min_ms, max_ms, mean_ms);
			ms = 0;
			frame_counter = 0;
		}
	}


	void drawCube (const btTransform& T)
	{
		btScalar m[16];
		T.getOpenGLMatrix (&m[0]);
		glPushMatrix ();
#ifdef BT_USE_DOUBLE_PRECISION
		glMultMatrixd (&m[0]);
			glScaled (2.0 * boxShapeHalfExtents[0], 2.0 * boxShapeHalfExtents[1], 2.0 * boxShapeHalfExtents[2]);
#else
			glMultMatrixf (&m[0]);
			glScalef (2.0 * boxShapeHalfExtents[0], 2.0 * boxShapeHalfExtents[1], 2.0 * boxShapeHalfExtents[2]);
#endif //BT_USE_DOUBLE_PRECISION
			glutSolidCube (1.0);
		glPopMatrix ();
	}

	void draw ()
	{
		glDisable (GL_LIGHTING);
		glColor3f (0.0, 1.0, 0.0);
		glBegin (GL_LINES);
		int i;
		for (i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			glVertex3f (source[i][0], source[i][1], source[i][2]);
			glVertex3f (hit_com[i][0], hit_com[i][1], hit_com[i][2]);
		}
		glColor3f (1.0, 1.0, 1.0);
		glBegin (GL_LINES);
		btScalar normal_scale = 10.0; // easier to see if this is big
		for (i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			glVertex3f (hit_surface[i][0], hit_surface[i][1], hit_surface[i][2]);
			glVertex3f (hit_surface[i][0] + normal_scale * normal[i][0], hit_surface[i][1] + normal_scale * normal[i][1], hit_surface[i][2] + normal_scale * normal[i][2]);
		}
		glEnd ();
		glColor3f (0.0, 1.0, 1.0);
		btQuaternion qFrom;
		btQuaternion qTo;
		qFrom.setRotation (btVector3(1.0, 0.0, 0.0), 0.0);
		qTo.setRotation (btVector3(1.0, 0.0, 0.0), 0.7);
		for ( i = 0; i < NUMRAYS_IN_BAR; i++)
		{
			btTransform from(qFrom, source[i]);
			btTransform to(qTo, dest[i]);
			btVector3 linVel, angVel;
			btTransformUtil::calculateVelocity (from, to, 1.0, linVel, angVel);
			btTransform T;
			btTransformUtil::integrateTransform (from, linVel, angVel, hit_fraction[i], T);
			drawCube (T);
		}
		glEnable (GL_LIGHTING);
	}
};


static btConvexcastBatch convexcastBatch;





const int NUM_VERTS_X = 30;
const int NUM_VERTS_Y = 30;
const int totalVerts = NUM_VERTS_X*NUM_VERTS_Y;

void	ConcaveConvexcastDemo::setVertexPositions(float waveheight, float offset)
{
	int i;
	int j;

	for ( i=0;i<NUM_VERTS_X;i++)
	{
		for (j=0;j<NUM_VERTS_Y;j++)
		{
			gVertices[i+j*NUM_VERTS_X].setValue((i-NUM_VERTS_X*0.5f)*TRIANGLE_SIZE,
				//0.f,
				waveheight*sinf((float)i+offset)*cosf((float)j+offset),
				(j-NUM_VERTS_Y*0.5f)*TRIANGLE_SIZE);
		}
	}
}

void ConcaveConvexcastDemo::keyboardCallback(unsigned char key, int x, int y)
{
	if (key == 'g')
	{
		m_animatedMesh = !m_animatedMesh;
		if (m_animatedMesh)
		{
			staticBody->setCollisionFlags( staticBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
			staticBody->setActivationState(DISABLE_DEACTIVATION);
		} else
		{
			staticBody->setCollisionFlags( staticBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
			staticBody->forceActivationState(ACTIVE_TAG);
		}
	}

	DemoApplication::keyboardCallback(key,x,y);

}

void	ConcaveConvexcastDemo::initPhysics()
{
	#define TRISIZE 10.f

	setCameraDistance(100.f);
   

	int vertStride = sizeof(btVector3);
	int indexStride = 3*sizeof(int);

	
	const int totalTriangles = 2*(NUM_VERTS_X-1)*(NUM_VERTS_Y-1);

	gVertices = new btVector3[totalVerts];
	gIndices = new int[totalTriangles*3];

	int i;


	setVertexPositions(waveheight,0.f);

	int index=0;
	for ( i=0;i<NUM_VERTS_X-1;i++)
	{
		for (int j=0;j<NUM_VERTS_Y-1;j++)
		{
			gIndices[index++] = j*NUM_VERTS_X+i;
			gIndices[index++] = j*NUM_VERTS_X+i+1;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i+1;

			gIndices[index++] = j*NUM_VERTS_X+i;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i+1;
			gIndices[index++] = (j+1)*NUM_VERTS_X+i;
		}
	}

	m_indexVertexArrays = new btTriangleIndexVertexArray(totalTriangles,
		gIndices,
		indexStride,
		totalVerts,(btScalar*) &gVertices[0].x(),vertStride);

	bool useQuantizedAabbCompression = true;

	trimeshShape  = new btBvhTriangleMeshShape(m_indexVertexArrays,useQuantizedAabbCompression);

	m_collisionShapes.push_back(trimeshShape);

	btCollisionShape* groundShape = trimeshShape;
	

	m_collisionConfiguration = new btDefaultCollisionConfiguration();

	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);

	btVector3 worldMin(-1000,-1000,-1000);
	btVector3 worldMax(1000,1000,1000);
	m_broadphase = new btAxisSweep3(worldMin,worldMax);
	m_solver = new btSequentialImpulseConstraintSolver();
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration);
	
	float mass = 0.f;
	btTransform	startTransform;
	startTransform.setIdentity();
	startTransform.setOrigin(btVector3(0,-2,0));

	btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
	m_collisionShapes.push_back(colShape);

	{
		for (int j=0;j<NUM_DYNAMIC_BOXES_X;j++)
		for (int i=0;i<NUM_DYNAMIC_BOXES_Y;i++)
		{
			//btCollisionShape* colShape = new btCapsuleShape(0.5,2.0);//boxShape = new btSphereShape(1.f);
			startTransform.setOrigin(btVector3(5*(i-NUM_DYNAMIC_BOXES_X/2),10,5*(j-NUM_DYNAMIC_BOXES_Y/2)));
			localCreateRigidBody(1, startTransform,colShape);
		}
	}

	startTransform.setIdentity();
	//startTransform = btTransform(btQuaternion (btVector3(1,1,1), 1.5));
	staticBody = localCreateRigidBody(mass, startTransform,groundShape);

	staticBody->setCollisionFlags(staticBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);

	//enable custom material callback
	staticBody->setCollisionFlags(staticBody->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	convexcastBatch = btConvexcastBatch (40.0, 0.0, -10.0,80.0);
	//convexcastBatch = btConvexcastBatch (true, 40.0, -50.0, 50.0);
}

void ConcaveConvexcastDemo::clientMoveAndDisplay()
{
	 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	float dt = getDeltaTimeMicroseconds() * 0.000001f;

	if (m_animatedMesh)
	{
		static float offset=0.f;
		offset+=0.01f;

		
		
		int i;
		int j;
		btVector3 aabbMin(BT_LARGE_FLOAT,BT_LARGE_FLOAT,BT_LARGE_FLOAT);
		btVector3 aabbMax(-BT_LARGE_FLOAT,-BT_LARGE_FLOAT,-BT_LARGE_FLOAT);

		for ( i=NUM_VERTS_X/2-3;i<NUM_VERTS_X/2+2;i++)
		{
			for (j=NUM_VERTS_X/2-3;j<NUM_VERTS_Y/2+2;j++)
			{
			
			aabbMax.setMax(gVertices[i+j*NUM_VERTS_X]);
			aabbMin.setMin(gVertices[i+j*NUM_VERTS_X]);
			
				gVertices[i+j*NUM_VERTS_X].setValue((i-NUM_VERTS_X*0.5f)*TRIANGLE_SIZE,
					//0.f,
					waveheight*sinf((float)i+offset)*cosf((float)j+offset),
					(j-NUM_VERTS_Y*0.5f)*TRIANGLE_SIZE);
					
			aabbMin.setMin(gVertices[i+j*NUM_VERTS_X]);
			aabbMax.setMax(gVertices[i+j*NUM_VERTS_X]);

			}
		}

		trimeshShape->partialRefitTree(aabbMin,aabbMax);


		//clear all contact points involving mesh proxy. Note: this is a slow/unoptimized operation.
		m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(staticBody->getBroadphaseHandle(),getDynamicsWorld()->getDispatcher());
	}

	m_dynamicsWorld->stepSimulation(dt);
	
	//optional but useful: debug drawing
	m_dynamicsWorld->debugDrawWorld();

	convexcastBatch.move (dt);
	convexcastBatch.cast (m_dynamicsWorld);
	renderme();
	convexcastBatch.draw ();
    glFlush();
    glutSwapBuffers();

}




void ConcaveConvexcastDemo::displayCallback(void) {

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

	renderme();
	convexcastBatch.draw ();
    glFlush();
    glutSwapBuffers();
}



void	ConcaveConvexcastDemo::exitPhysics()
{



	//cleanup in the reverse order of creation/initialization

	//remove the rigidbodies from the dynamics world and delete them
	int i;
	for (i=m_dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
	{
		btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		m_dynamicsWorld->removeCollisionObject( obj );
		delete obj;
	}

	//delete collision shapes
	for (int j=0;j<m_collisionShapes.size();j++)
	{
		btCollisionShape* shape = m_collisionShapes[j];
		delete shape;
	}

	//delete dynamics world
	delete m_dynamicsWorld;

	if (m_indexVertexArrays)
		delete m_indexVertexArrays;

	//delete solver
	delete m_solver;

	//delete broadphase
	delete m_broadphase;

	//delete dispatcher
	delete m_dispatcher;

	delete m_collisionConfiguration;

	
}




