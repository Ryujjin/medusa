/* NodeRealStarField.cpp - Session 97: fixed UV sphere winding, view-space stars, pushIdentity sphere. (c)2025 */
#define RENDER3D_DLL
#include "Math/Helpers.h"
#include "Math/Vector3.h"
#include "Math/Matrix33.h"
#include "Display/Types.h"
#include "Display/Color.h"
#include "Display/PrimitiveMaterial.h"
#include "Display/PrimitiveTriangleList.h"
#include "Display/PrimitiveGalaxy.h"
#include "Display/PrimitiveGalaxySpriteSet.h"
#include "Display/PrimitiveStarSurface.h"
#include "Render3D/RenderContext.h"
#include "Render3D/NodeRealStarField.h"
#include "Render3D/StarCatalog.h"
#include "Debug/Trace.h"
#include "Standard/CharString.h"
#include <math.h>
#include <string.h>
float    NodeRealStarField::s_StarSize = 0.001f;
Matrix33 NodeRealStarField::s_CelestialFrame;
static const float PI_F = 3.14159265f;
static Vector3 raDecToGame(float ra_h,float dec_d,const Matrix33 &m){
    float ra=ra_h*(PI_F/12.0f),dec=dec_d*(PI_F/180.0f);
    float x=cosf(dec)*cosf(ra),y=cosf(dec)*sinf(ra),z=sinf(dec);
    return Vector3(x*m.i.x+y*m.j.x+z*m.k.x,x*m.i.y+y*m.j.y+z*m.k.y,x*m.i.z+y*m.j.z+z*m.k.z);
}
static float starHashF(float a,float b){
    unsigned int ia,ib;memcpy(&ia,&a,4);memcpy(&ib,&b,4);
    unsigned int h=(ia^0xDEADBEEFu)+ib;h^=(h>>16);h*=0x45d9f3bu;h^=(h>>16);
    return (float)(h&0x3FFFFFFFu)/(float)0x3FFFFFFFu;
}
NodeRealStarField::NodeRealStarField()
:m_bActive(true),m_bTrailActive(false),m_bJumpActive(false)
,m_fBack(500.0f),m_bBuilt(false),m_bLocalSet(false)
,m_bLocalWorldPosSet(false),m_fLocalStarRadius(0.0f){
    m_LocalStarName[0]='\0';m_StarColor=Vector3(1,1,1);
    m_LocalStarDir=Vector3(1,0,0);m_LocalStarWorldPos=Vector3(0,0,0);
    static bool s_init=false;if(!s_init){s_init=true;
    s_CelestialFrame.i=Vector3(1,0,0);s_CelestialFrame.j=Vector3(0,0,1);s_CelestialFrame.k=Vector3(0,1,0);}
}
void NodeRealStarField::setLocalSystem(const char*starName){
    if(!starName){if(m_LocalStarName[0]=='\0')return;m_LocalStarName[0]='\0';m_bLocalSet=false;m_bBuilt=false;return;}
    if(_strnicmp(m_LocalStarName,starName,sizeof(m_LocalStarName)-1)==0)return;
    strncpy(m_LocalStarName,starName,sizeof(m_LocalStarName)-1);
    m_LocalStarName[sizeof(m_LocalStarName)-1]='\0';m_bLocalSet=true;m_bBuilt=false;
}
void NodeRealStarField::buildCatalog(){
    m_bBuilt=true;m_Stars.release();m_GalaxySprites.release();
    for(int i=0;i<g_StarCatalogCount;++i){
        const StarEntry&se=g_StarCatalog[i];
        if(m_bLocalSet&&se.name&&_stricmp(se.name,m_LocalStarName)==0){
            m_StarColor=Vector3(se.r/255.0f,se.g/255.0f,se.b/255.0f);
            m_LocalStarDir=raDecToGame(se.ra_h,se.dec_d,s_CelestialFrame);m_LocalStarDir.normalize();continue;
        }
        CatalogStar cs;
        cs.dir=raDecToGame(se.ra_h,se.dec_d,s_CelestialFrame);cs.dir.normalize();
        float bright=powf(10.0f,-0.4f*(se.mag_v-6.0f));if(bright>1.0f)bright=1.0f;
        cs.color=Color(se.r,se.g,se.b,(unsigned char)(bright*255.0f));
        cs.size=s_StarSize*(1.0f+2.5f*bright);cs.flickerPhase=starHashF((float)i,(float)(i*7+3));
        cs.name=se.name;m_Stars.push(cs);
    }
    for(int i=0;i<g_ExtendedObjectCount;++i){
        const ExtendedObject&eo=g_ExtendedObjects[i];
        CatalogStar cs;
        cs.dir=raDecToGame(eo.ra_h,eo.dec_d,s_CelestialFrame);cs.dir.normalize();
        float bright=powf(10.0f,-0.4f*(eo.mag_v-6.0f));if(bright>1.0f)bright=1.0f;
        cs.color=Color(eo.r,eo.g,eo.b,(unsigned char)(bright*255.0f));
        cs.size=s_StarSize*eo.size_mult;cs.flickerPhase=-1.0f;cs.name=eo.name;m_Stars.push(cs);
    }
    for(int i=0;i<g_GalaxySpriteCount;++i){
        const GalaxySpriteEntry&gs=g_GalaxySprites[i];
        GalaxySpriteStar gss;
        gss.dir=raDecToGame(gs.ra_h,gs.dec_d,s_CelestialFrame);gss.dir.normalize();
        gss.brightness=0.8f;
        const float MARGIN=0.02f,COLS=7.0f,ROWS=6.0f;
        gss.u0=(gs.atlas_col+MARGIN)/COLS;gss.v0=(gs.atlas_row+MARGIN)/ROWS;
        gss.u1=(gs.atlas_col+1.0f-MARGIN)/COLS;gss.v1=(gs.atlas_row+1.0f-MARGIN)/ROWS;
        gss.size=s_StarSize*m_fBack*gs.size_mult;m_GalaxySprites.push(gss);
    }
    m_GnpDir=Vector3(0.0f,1.0f,0.0f);
    m_GalCenDir=raDecToGame(17.761f,-28.936f,s_CelestialFrame);
    float d=m_GalCenDir.x*m_GnpDir.x+m_GalCenDir.y*m_GnpDir.y+m_GalCenDir.z*m_GnpDir.z;
    m_GalCenDir.x-=d*m_GnpDir.x;m_GalCenDir.y-=d*m_GnpDir.y;m_GalCenDir.z-=d*m_GnpDir.z;
    m_GalCenDir.normalize();
}
void NodeRealStarField::render(RenderContext&context,const Matrix33&frame,const Vector3&position){
    if(!m_bActive)return;
    DisplayDevice*pDisplay=context.display();if(!pDisplay)return;
    if(!m_bBuilt)buildCatalog();
    TRACE(CharString().format("NodeRealStarField::render() m_bLocalSet=%d m_bLocalWorldPosSet=%d starName=%s radius=%.1f",(int)m_bLocalSet,(int)m_bLocalWorldPosSet,m_LocalStarName,m_fLocalStarRadius));
    const Vector3 camPos=context.position();const Matrix33 camFrame=context.frame();
    const int nStars=m_Stars.size();
    if(nStars>0){
        // View-space 3-vertex triangles like NodeStarField.
        // pushIdentity sets mWorld=I; vertex positions are in VIEW space so the GPU
        // camera transform mView is the identity pass-through for these verts.
        const Vector3 N(0.0f,0.0f,-1.0f);
        const Vector3 vRight(1.0f,0.0f,0.0f);
        const Vector3 vUp(0.0f,1.0f,0.0f);
        PrimitiveTriangleListDL::Ref pTL=PrimitiveTriangleListDL::create(pDisplay,nStars);
        VertexL*pVertex=(VertexL*)pTL->lock();
        float t=context.time();
        for(int i=0;i<nStars;++i){
            const CatalogStar&cs=m_Stars[i];
            Vector3 vsPos=context.worldToView(camPos+cs.dir*m_fBack);
            float fDepth=vsPos.z;
            if(fDepth<=0.0f){
                // behind camera - degenerate triangle
                pVertex[0].position=pVertex[1].position=pVertex[2].position=Vector3(0,0,1);
                pVertex[0].normal=pVertex[1].normal=pVertex[2].normal=N;
                pVertex[0].diffuse=pVertex[1].diffuse=pVertex[2].diffuse=0;
                pVertex[0].u=pVertex[1].u=pVertex[2].u=0;
                pVertex[0].v=pVertex[1].v=pVertex[2].v=0;
                pVertex+=3;continue;
            }
            float sz=s_StarSize*fDepth;
            Color col=cs.color;
            if(cs.flickerPhase>=0.0f){
                float ph=cs.flickerPhase*6.28318f;
                float fl=0.70f+0.30f*sinf(t*3.7f+ph)*(0.5f+0.5f*sinf(t*11.3f+ph*1.4f));
                col.a=(unsigned char)(col.a*fl);
            }
            unsigned int dif=((unsigned int)col.a<<24)|((unsigned int)col.r<<16)|((unsigned int)col.g<<8)|(unsigned int)col.b;
            // u=flickerPhase for per-star twinkle timing; v=2.0 triggers VS_MainL brightness
            // modulation path.  Extended objects use fixed phase 0.5 (no flicker, just glow).
            // Normal.xy encodes 2D offset from billboard center (used by PS_Main Gaussian fade):
            //   Vertex 0 (top):   normal=( 0,-1, 0)
            //   Vertex 1 (right): normal=(+1,+1, 0)
            //   Vertex 2 (left):  normal=(-1,+1, 0)
            // Normal.z=0 distinguishes star verts from geometry verts (which have real normals).
            float uPhase=(cs.flickerPhase>=0.0f)?cs.flickerPhase:0.5f;
            pVertex[0].position=vsPos+vUp*sz;    pVertex[0].normal=Vector3( 0,-1,0);pVertex[0].diffuse=dif;pVertex[0].u=uPhase;pVertex[0].v=2.0f;
            pVertex[1].position=vsPos+vRight*sz; pVertex[1].normal=Vector3(+1,+1,0);pVertex[1].diffuse=dif;pVertex[1].u=uPhase;pVertex[1].v=2.0f;
            pVertex[2].position=vsPos-vRight*sz; pVertex[2].normal=Vector3(-1,+1,0);pVertex[2].diffuse=dif;pVertex[2].u=uPhase;pVertex[2].v=2.0f;
            pVertex+=3;
        }
        pTL->unlock();
        context.push(PrimitiveMaterial::create(pDisplay,PrimitiveMaterial::ADDITIVE,false,true));
        context.pushIdentity();
        context.push(pTL);
    }
    context.push(PrimitiveMaterial::create(pDisplay,PrimitiveMaterial::ADDITIVE,false,true));
    context.pushIdentity();
    context.push(PrimitiveGalaxy::create(pDisplay,m_GnpDir,m_GalCenDir));
    const int nSpr=m_GalaxySprites.size();
    if(nSpr>0){
        PrimitiveGalaxySpriteSet::SpriteVertex*pSV=new PrimitiveGalaxySpriteSet::SpriteVertex[nSpr*6];
        int vi=0;
        for(int i=0;i<nSpr;++i){
            const GalaxySpriteStar&gss=m_GalaxySprites[i];
            Vector3 cen=camPos+gss.dir*m_fBack;
            float sz=gss.size;
            Vector3 r=camFrame.i*sz,u=camFrame.j*sz;
            Vector3 tl=cen-r+u,tr=cen+r+u,bl=cen-r-u,brc=cen+r-u;
            unsigned char ba=(unsigned char)(gss.brightness*200.0f);
            unsigned int dif=((unsigned int)ba<<24)|0x00FFFFFFu;
            auto SV=[&](Vector3 p,float uu,float vv)->PrimitiveGalaxySpriteSet::SpriteVertex{
                PrimitiveGalaxySpriteSet::SpriteVertex sv;
                sv.x=p.x;sv.y=p.y;sv.z=p.z;sv.nx=0;sv.ny=0;sv.nz=0;sv.diffuse=dif;sv.u=uu;sv.v=vv;return sv;
            };
            pSV[vi  ]=SV(tl, gss.u0,gss.v0);pSV[vi+1]=SV(tr, gss.u1,gss.v0);pSV[vi+2]=SV(brc,gss.u1,gss.v1);
            pSV[vi+3]=SV(tl, gss.u0,gss.v0);pSV[vi+4]=SV(brc,gss.u1,gss.v1);pSV[vi+5]=SV(bl, gss.u0,gss.v1);
            vi+=6;
        }
        context.push(PrimitiveMaterial::create(pDisplay,PrimitiveMaterial::ADDITIVE,false,true));
        context.pushIdentity();
        context.push(PrimitiveGalaxySpriteSet::create(pDisplay,pSV,nSpr*6));
        delete[]pSV;
    }
    if(m_bLocalSet){
        Vector3 starDir;
        float camToStarDist=0.0f;
        if(m_bLocalWorldPosSet){
            Vector3 d=m_LocalStarWorldPos-camPos;
            camToStarDist=sqrtf(d.x*d.x+d.y*d.y+d.z*d.z);
            starDir=(camToStarDist>0.001f)?d*(1.0f/camToStarDist):m_LocalStarDir;
        }else{starDir=m_LocalStarDir;}

        // Physical radius of the photosphere sphere.
        float sphereRadius;
        if(m_fLocalStarRadius>0.0f)
            sphereRadius=m_fLocalStarRadius;
        else
            sphereRadius=s_StarSize*m_fBack*28.0f;

        // Place the sphere meshes at the star's REAL world distance whenever it is known.
        // A previous "fixed proxy distance" hack (sphereDist = m_fBack, ~500 units) forced
        // the corona/photosphere spheres to always render extremely close to the camera,
        // regardless of the star's true distance. Because ADDITIVE geometry here uses
        // depth-test-only (no write), that made the corona ALWAYS win the depth test against
        // any real scene object (planet, jump gate, ship) that happened to lie in roughly the
        // same screen direction as the star -- producing a halo/shell that appears, partially
        // covers, or vanishes around nearby objects purely as a function of camera orbit angle
        // (the star's screen-space direction sweeping across the object's silhouette), NOT
        // actual 3D occlusion. Using the real distance restores correct depth relationships:
        // objects that are actually closer to the camera than the star now correctly occlude
        // the corona, and vice versa. The angular-size cap below still prevents the corona
        // from growing to an unreasonable screen size at very large distances.
        float sphereDist = (m_bLocalWorldPosSet && camToStarDist > 0.001f) ? camToStarDist : m_fBack;
        Vector3 sphereCenter = camPos + starDir * sphereDist;
        float discRadius = sphereRadius;
        // Angular-size cap tightened again (Session 133): the corona SHELL geometry
        // (2.5x this radius) still had enough absolute size at moderate zoom levels
        // to physically poke closer to the camera than nearby objects (jump gates,
        // planets) that happened to sit near the star's screen direction, producing
        // extra visible halo rings around them even with depth-testing enabled
        // (the shell's near surface is genuinely nearer than the object in that case).
        // Shrinking the cap reduces the shell's absolute radius so it can no longer
        // reach far enough toward the camera to intersect nearby scenery.
        const float maxDisc = 0.028f * sphereDist;   // ~1.6-degree half-angle cap (was 0.0699f/4deg)
        if(discRadius > maxDisc) discRadius = maxDisc;

        // Absolute world-unit ceiling (Session 134 fix): the angular cap above only
        // shrinks discRadius RELATIVE to camera-to-star distance -- it has no bearing
        // on how far the star actually is from OTHER scene objects (planets, jump
        // gates) that may sit near it. If m_fLocalStarRadius (NounStar::radius()) is
        // large for a given system, or the camera is far enough that the angular cap
        // doesn't engage, the shell sphere is a REAL 3D object centered on the star's
        // true world position and can physically extend far enough to geometrically
        // swallow any nearby object -- producing the halo on "certain planets/objects"
        // (near or far side of the star) and jump gates, regardless of object type,
        // because this is true 3D sphere/point overlap, not a screen-space artifact.
        // Clamp to a small fixed absolute radius so the corona can never reach nearby
        // scenery no matter how large the reported star radius or camera distance is.
        const float ABS_MAX_STAR_RADIUS = 150.0f;
        if(discRadius > ABS_MAX_STAR_RADIUS) discRadius = ABS_MAX_STAR_RADIUS;

        // Shell sphere radius reduced from 2.5x to 1.6x photosphere radius so the
        // corona shell's near surface stays much closer to the photosphere itself
        // and cannot bleed over nearby objects (must match shader SHELL_RATIO_INV below).
        const float shellRadius = discRadius * 1.6f; // must match shader SHELL_RATIO_INV=0.625 (1/1.6)

        // Build a shared UV sphere mesh at photosphere scale (discRadius).
        // The same mesh is reused for the corona shell by scaling vertex positions.
        // 24 stacks × 48 slices = 2304 triangles.  CW from outside (FrontCounterClockwise=FALSE).
        // Vertex positions are in WORLD space; pushIdentity sets mWorld=I.
        const int STACKS=24, SLICES=48, nTri=STACKS*SLICES*2;

        // --- Photosphere sphere ---
        {
            Triangle* pTris = new Triangle[nTri]; int ti=0;
            for(int stack=0;stack<STACKS;++stack){
                float phi0=PI_F*(float)stack/(float)STACKS;
                float phi1=PI_F*(float)(stack+1)/(float)STACKS;
                float tv0=(float)stack/(float)STACKS, tv1=(float)(stack+1)/(float)STACKS;
                for(int slice=0;slice<SLICES;++slice){
                    float th0=2.0f*PI_F*(float)slice/(float)SLICES;
                    float th1=2.0f*PI_F*(float)(slice+1)/(float)SLICES;
                    float tu0=(float)slice/(float)SLICES, tu1=(float)(slice+1)/(float)SLICES;
                    Vector3 n00(sinf(phi0)*cosf(th0),cosf(phi0),sinf(phi0)*sinf(th0));
                    Vector3 n01(sinf(phi0)*cosf(th1),cosf(phi0),sinf(phi0)*sinf(th1));
                    Vector3 n10(sinf(phi1)*cosf(th0),cosf(phi1),sinf(phi1)*sinf(th0));
                    Vector3 n11(sinf(phi1)*cosf(th1),cosf(phi1),sinf(phi1)*sinf(th1));
                    Vector3 p00=sphereCenter+n00*discRadius;
                    Vector3 p01=sphereCenter+n01*discRadius;
                    Vector3 p10=sphereCenter+n10*discRadius;
                    Vector3 p11=sphereCenter+n11*discRadius;
                    pTris[ti  ].v[0]=Vertex(p00,n00,tu0,tv0);
                    pTris[ti  ].v[1]=Vertex(p01,n01,tu1,tv0);
                    pTris[ti  ].v[2]=Vertex(p11,n11,tu1,tv1);
                    pTris[ti+1].v[0]=Vertex(p00,n00,tu0,tv0);
                    pTris[ti+1].v[1]=Vertex(p11,n11,tu1,tv1);
                    pTris[ti+1].v[2]=Vertex(p10,n10,tu0,tv1);
                    ti+=2;
                }
            }
            // bDoubleSided=false (back-face culled): Session 126 root-cause fix.
            // With no culling (bDoubleSided=true), the BACK hemisphere of this sphere
            // (facing away from camera, toward any nearby object on the far side of the
            // star) composites additively over that object -- producing exactly the
            // "halo/shell" bleed-through artifact seen on planets and the ICC Jump Gate.
            // Single-sided + back-face culling removes the far hemisphere entirely.
            context.push(PrimitiveMaterial::create(pDisplay,PrimitiveMaterial::ADDITIVE,false,false));
            context.pushIdentity();
            context.push(createPrimitiveStarSurface(pDisplay,pTris,nTri,m_StarColor,false));
            delete[]pTris;
        }

        // --- Corona shell sphere (3D geometry, PS_CoronaShell) ---
        // Rendered after the photosphere so additive blending accumulates correctly.
        {
            Triangle* pTris = new Triangle[nTri]; int ti=0;
            for(int stack=0;stack<STACKS;++stack){
                float phi0=PI_F*(float)stack/(float)STACKS;
                float phi1=PI_F*(float)(stack+1)/(float)STACKS;
                float tv0=(float)stack/(float)STACKS, tv1=(float)(stack+1)/(float)STACKS;
                for(int slice=0;slice<SLICES;++slice){
                    float th0=2.0f*PI_F*(float)slice/(float)SLICES;
                    float th1=2.0f*PI_F*(float)(slice+1)/(float)SLICES;
                    float tu0=(float)slice/(float)SLICES, tu1=(float)(slice+1)/(float)SLICES;
                    Vector3 n00(sinf(phi0)*cosf(th0),cosf(phi0),sinf(phi0)*sinf(th0));
                    Vector3 n01(sinf(phi0)*cosf(th1),cosf(phi0),sinf(phi0)*sinf(th1));
                    Vector3 n10(sinf(phi1)*cosf(th0),cosf(phi1),sinf(phi1)*sinf(th0));
                    Vector3 n11(sinf(phi1)*cosf(th1),cosf(phi1),sinf(phi1)*sinf(th1));
                    Vector3 p00=sphereCenter+n00*shellRadius;
                    Vector3 p01=sphereCenter+n01*shellRadius;
                    Vector3 p10=sphereCenter+n10*shellRadius;
                    Vector3 p11=sphereCenter+n11*shellRadius;
                    pTris[ti  ].v[0]=Vertex(p00,n00,tu0,tv0);
                    pTris[ti  ].v[1]=Vertex(p01,n01,tu1,tv0);
                    pTris[ti  ].v[2]=Vertex(p11,n11,tu1,tv1);
                    pTris[ti+1].v[0]=Vertex(p00,n00,tu0,tv0);
                    pTris[ti+1].v[1]=Vertex(p11,n11,tu1,tv1);
                    pTris[ti+1].v[2]=Vertex(p10,n10,tu0,tv1);
                    ti+=2;
                }
            }
            context.push(PrimitiveMaterial::create(pDisplay,PrimitiveMaterial::ADDITIVE,false,false));
            context.pushIdentity();
            context.push(createPrimitiveStarSurface(pDisplay,pTris,nTri,m_StarColor,true));
            delete[]pTris;
        }
    }
}
const char*NodeRealStarField::nearestName(const Vector3&worldDir,float dot_threshold)const{
    Vector3 dir=worldDir;dir.normalize();
    float bestDot=dot_threshold;const char*bestName=NULL;
    for(int i=0;i<m_Stars.size();++i){
        if(!m_Stars[i].name)continue;
        float d=m_Stars[i].dir.x*dir.x+m_Stars[i].dir.y*dir.y+m_Stars[i].dir.z*dir.z;
        if(d>bestDot){bestDot=d;bestName=m_Stars[i].name;}
    }
    return bestName;
}
// EOF
