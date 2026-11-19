/**
	@file ImportFBX.cpp

	(c)2006 Palestar Inc
	@author Richard Lyle @date 10/27/2008 11:16:02 PM
*/

#define WINVER 0xA00

#include "File/Path.h"
#include "File/FileDisk.h"
#include "Debug/Trace.h"

#undef TRACE		// avoid warning with MFC
#undef ASSERT

#include "ImportFBX.h"
#include "Tools/ResourcerDoc/ResourcerDoc.h"
#include "Tools/NodeComplexMeshPort/Port.h"
#include "Tools/ResourcerDoc/Port.h"
#include "Tools/MaterialPort/Port.h"
#include "Tools/ImagePort/Port.h"

#include <map>
#include <vector>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

//---------------------------------------------------------------------------------------------------

ImportFBX FBX_IMPORTER;

// convert an Assimp matrix into a Matrix33 for orientation and a Vector3 for transform
static void AssimpToMedusa( const aiMatrix4x4 & m, Matrix33 & f, Vector3 & t )
{
    f.i.x = m.a1;  f.i.y = m.b1;  f.i.z = m.c1;
    f.j.x = m.a2;  f.j.y = m.b2;  f.j.z = m.c2;
    f.k.x = m.a3;  f.k.y = m.b3;  f.k.z = m.c3;
    t.x = m.a4;    t.y = m.b4;    t.z = m.c4;
}

static Color ConvertColor( const aiColor3D & c )
{
    return Color( (u8)(255 * Clamp<float>( c.r, 0.0f, 1.0f )), 
        (u8)(255 * Clamp<float>( c.g, 0.0f, 1.0f )), 
        (u8)(255 * Clamp<float>( c.b, 0.0f, 1.0f )) );
}



//---------------------------------------------------------------------------------------------------

ImportFBX::ImportFBX() : m_pScene( NULL )
{}

ImportFBX::~ImportFBX()
{}

bool ImportFBX::import( ScenePort * a_pScene, const char * a_pFileName )
{
	Path p( a_pFileName );
	if ( stricmp( p.extension(), ".fbx" ) != 0				// Filmbox
		&& stricmp( p.extension(), ".dae" ) != 0			// Collada DAE
		&& stricmp( p.extension(), ".max" ) != 0			// 3D studio Max
		&& stricmp( p.extension(), ".3ds" ) != 0			// 3D studio Max
		&& stricmp( p.extension(), ".dxf" ) != 0			// Autodesk
		&& stricmp( p.extension(), ".obj" ) != 0			// Alias OBJ
	)
	{
		// not a supported extension, early out...
		return false;
	}

	m_sImportFile = a_pFileName;
	m_sBaseDirectory = Path( a_pFileName ).directory();

	CharString sImportPath( CResourcerDoc::document()->currentDirectory() );
	//CharString sMaterialPath( sImportPath + "Materials\\" );
	//CreateDirectory( sMaterialPath, NULL );
	//CharString sTexturesPath( importPath + "Textures\\" );
	//CreateDirectory( sTexturesPath, NULL );

	Assimp::Importer importer;
	const aiScene * pScene = importer.ReadFile( a_pFileName, 
		aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals );
	if ( pScene == NULL || pScene->mRootNode == NULL )
	{
		a_pScene->report( CharString().format( "Failed to load (%s): %s", a_pFileName, importer.GetErrorString() ) );
		return false;
	}

	m_pScene = pScene;
	bool bImported = importNodes( a_pScene, a_pScene->m_Root, pScene->mRootNode );
	m_pScene = NULL;

	return bImported;
}

//---------------------------------------------------------------------------------------------------

bool ImportFBX::importNodes( ScenePort * a_pScene, 
						BaseNodePort * a_pParent, 
						const aiNode * a_pNode )
{
	if (! a_pNode || !a_pParent )
		return false;

	NodePort * pNode = NULL;
	if ( a_pNode->mNumMeshes > 0 )
		pNode = importMesh( a_pScene, a_pParent, a_pNode );
	else
		pNode = importNode( a_pScene, a_pParent, a_pNode );

	if ( pNode )
	{
		// keep the names the same
		pNode->setName( a_pNode->mName.C_Str() );
		// attach imported node 
		a_pParent->m_Children.push( pNode );

		// recurse into any child nodes..
		for( unsigned int i=0;i<a_pNode->mNumChildren;++i )
		{
			if (! importNodes( a_pScene, pNode, a_pNode->mChildren[i] ) )
				return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------------------------------

NodePort * ImportFBX::importNode( ScenePort * a_pScene,
						BaseNodePort * a_pParent,
						const aiNode * a_pNode )
{
	NodePort * pOut = new NodePort();
	AssimpToMedusa( a_pNode->mTransformation, pOut->m_Frame, pOut->m_Position );

	return pOut;
}
								
NodePort * ImportFBX::importMesh( ScenePort * a_pScene,
						BaseNodePort * a_pParent,
						const aiNode * a_pNode )
{
	const char * pNodeName = a_pNode->mName.C_Str();

	// collect the triangle meshes of this node, each distinct material gets a slot
	std::vector<const aiMesh *> meshes;
	unsigned int nTotalVerts = 0;
	for( unsigned int i=0;i<a_pNode->mNumMeshes;++i )
	{
		const aiMesh * pMesh = m_pScene->mMeshes[ a_pNode->mMeshes[i] ];
		if ( (pMesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0 )
			continue;
		meshes.push_back( pMesh );
		nTotalVerts += pMesh->mNumVertices;
	}

	// triangle indices are 16 bit words, all verts of the node must be addressable
	if ( nTotalVerts > 65536 )
	{
		a_pScene->report( CharString().format( "Node (%s) has %u vertices, exceeds 65536 limit", pNodeName, nTotalVerts ) );
		return NULL;
	}

	NodeComplexMeshPort * pOut = new NodeComplexMeshPort();

	std::map<unsigned int, int> materialSlots;
	for( size_t i=0;i<meshes.size();++i )
	{
		unsigned int nMaterialIndex = meshes[i]->mMaterialIndex;
		if ( materialSlots.find( nMaterialIndex ) == materialSlots.end() )
		{
			materialSlots[ nMaterialIndex ] = (int)pOut->m_Materials.size();
			pOut->m_Materials.push( importMaterial( a_pScene, m_pScene->mMaterials[ nMaterialIndex ], pNodeName ) );
		}
	}

	// we must have at least one material, create a default one if needed..
	if ( pOut->m_Materials.size() == 0 )
		pOut->m_Materials.push( importMaterial( a_pScene, NULL, pNodeName ) );

	pOut->m_Triangles.allocate( 1 );
	pOut->m_Triangles[0].allocate( pOut->m_Materials.size() );

	for( size_t i=0;i<meshes.size();++i )
	{
		const aiMesh * pMesh = meshes[i];
		int nMaterial = materialSlots[ pMesh->mMaterialIndex ];
		word nBaseVertex = (word)pOut->m_Verts.size();

		for( unsigned int v=0;v<pMesh->mNumVertices;++v )
		{
			const aiVector3D & position = pMesh->mVertices[v];

			Vertex vertex;
			vertex.position = Vector3( position.x, position.y, position.z );
			if ( pMesh->mNormals )
			{
				const aiVector3D & normal = pMesh->mNormals[v];
				vertex.normal = Vector3( normal.x, normal.y, normal.z );
			}
			else
				vertex.normal = Vector3( 0.0f, 0.0f, 0.0f );

			vertex.u = vertex.v = 0.0f;
			if ( pMesh->HasTextureCoords( 0 ) )
			{
				const aiVector3D & uv = pMesh->mTextureCoords[0][v];
				vertex.u = uv.x;
				vertex.v = 1.0f - fmod( uv.y, 1.0f );
			}

			pOut->m_Verts.push( vertex );
		}

		for( unsigned int f=0;f<pMesh->mNumFaces;++f )
		{
			const aiFace & face = pMesh->mFaces[f];
			if ( face.mNumIndices != 3 )
				continue;		// only triangles are supported

			NodeComplexMesh2::Triangle & triangle = pOut->m_Triangles[0][nMaterial].push();
			for( unsigned int j=0;j<3;++j )
				triangle.verts[j] = (word)(nBaseVertex + face.mIndices[j]);
		}
	}

	// default to optimize the mesh...
	pOut->m_Optimize = true;
	pOut->m_CalcNormals = true;
	pOut->m_Reverse = true;

	// get the frame & position, then transform our verts..
	AssimpToMedusa( a_pNode->mTransformation, pOut->m_Frame, pOut->m_Position );
	pOut->transformVerts();

	return pOut;
}

CharString ImportFBX::importMaterial( ScenePort * a_pScene, const aiMaterial * a_pMaterial, const char * a_pDefaultName )
{
	CharString sImportPath( CResourcerDoc::document()->currentDirectory() );
	CharString sMaterialName( a_pDefaultName );

	aiString name;
	if ( a_pMaterial != NULL && a_pMaterial->Get( AI_MATKEY_NAME, name ) == AI_SUCCESS && name.length > 0 )
		sMaterialName = name.C_Str();

	CharString sMaterialPort( sImportPath + "m_" + sMaterialName + ".prt" );
	if (! FileDisk::fileExists( sMaterialPort ) )
	{
		// create material 
		MaterialPort::Ref pMaterialPort = new MaterialPort();

		if ( a_pMaterial != NULL )
		{
			aiColor3D color;
			if ( a_pMaterial->Get( AI_MATKEY_COLOR_AMBIENT, color ) == AI_SUCCESS )
				pMaterialPort->m_Ambient = ConvertColor( color );
			if ( a_pMaterial->Get( AI_MATKEY_COLOR_DIFFUSE, color ) == AI_SUCCESS )
				pMaterialPort->m_Diffuse = ConvertColor( color );
			if ( a_pMaterial->Get( AI_MATKEY_COLOR_EMISSIVE, color ) == AI_SUCCESS )
				pMaterialPort->m_Emissive = ConvertColor( color );

			aiString texture;
			if ( a_pMaterial->GetTexture( aiTextureType_DIFFUSE, 0, &texture ) == AI_SUCCESS )
			{
				pMaterialPort->m_Textures.push( MaterialPort::Texture( PrimitiveSurface::DIFFUSE, 
					importImage( a_pScene, texture.C_Str() ), true ) );
			}

			if ( a_pMaterial->GetTexture( aiTextureType_EMISSIVE, 0, &texture ) == AI_SUCCESS )
			{
				pMaterialPort->m_Textures.push( MaterialPort::Texture( PrimitiveSurface::LIGHTMAP, 
					importImage( a_pScene, texture.C_Str() ), true ) );
			}

			// normal maps are stored as NORMALS, older formats store bump maps as HEIGHT
			if ( a_pMaterial->GetTexture( aiTextureType_NORMALS, 0, &texture ) == AI_SUCCESS
				|| a_pMaterial->GetTexture( aiTextureType_HEIGHT, 0, &texture ) == AI_SUCCESS )
			{
				pMaterialPort->m_Textures.push( MaterialPort::Texture( PrimitiveSurface::BUMPMAP, 
					importImage( a_pScene, texture.C_Str() ), true ) );
			}
		}

		// save the material to disk
		bool bSaved = false;
		try {
			OutStream( new FileDisk( sMaterialPort, FileDisk::READ_WRITE ) ) << pMaterialPort;
			bSaved = true;
		}
		catch( File::FileError )
		{}
	}

	return CResourcerDoc::document()->relative( sMaterialPort );
}

static bool FindFileRecursively( const CharString & a_sPath, const CharString & a_sMask, CharString & a_sFile )
{
	CharString sPath( a_sPath );
	if (! sPath.endsWith( PATH_SEPERATOR ) )
		sPath += PATH_SEPERATOR;

	// search for our specific file in this directory..
	FindFile ff( sPath + a_sMask, false, true );
	if ( ff.fileCount() > 0 )
	{
		a_sFile = ff.file(0);
		return true;
	}

	// recurse into any subdirectories as well..
	ff.findFiles( sPath + "*.*" );
	for(int i=0;i<ff.directoryCount();++i)
	{
		if ( ff.directory(i)[0] == '.' )
			continue;
		if ( FindFileRecursively( sPath + ff.directory(i), a_sMask, a_sFile ) )
			return true;
	}

	return false;
}

CharString ImportFBX::importImage( ScenePort * a_pScene, const char * a_pFile )
{
	if (! a_pFile || a_pFile[0] == 0 )
		return "";

	CharString sImportPath( CResourcerDoc::document()->currentDirectory() );
	CharString sImagePort( sImportPath + "i_" + Path( a_pFile ).name() + ".prt" );

	if (! FileDisk::fileExists( sImagePort ) )
	{
		CharString sImportFile( a_pFile );

		ImagePort::Ref pImagePort = new ImagePort();
		if (! FileDisk::fileExists( sImportFile ) )
		{
			// get only the file part..
			sImportFile = Path( sImportFile ).file();

			// image file not found, try to find it by searching each directory recusrively
			// from the base path..
			CharString sFindPath( m_sBaseDirectory );
			sFindPath.replace( '/','\\' );

			CharString sFoundFile;
			if (! FindFileRecursively( sFindPath, sImportFile, sFoundFile ) )
				return "";		// failed to find the file..

			sImportFile = sFoundFile;

			//int nLastSlash = sFindPath.reverseFind( '\\' );
			//while( nLastSlash >= 0 )
			//{
			//	sFindPath = sFindPath.left( nLastSlash );
			//	nLastSlash = sFindPath.reverseFind( '\\' );

			//	CharString sFoundFile;
			//	if ( FindFileRecursively( sFindPath, sImportFile, sFoundFile ) )
			//	{
			//		sImportFile = sFoundFile;
			//		break;
			//	}
			//}

			//if ( nLastSlash < 0 )
			//	return "";		// failed to find the texture!
		}

		if (! pImagePort->importFile( sImportFile ) )
			a_pScene->report( CharString().format( "Failed to import image (%s)!", sImportFile ) );

		bool bSaved = false;
		try {
			OutStream( new FileDisk( sImagePort, FileDisk::READ_WRITE ) ) << pImagePort;
			bSaved = true;
		}
		catch( File::FileError )
		{}
	}

	return CResourcerDoc::document()->relative( sImagePort );
}

//---------------------------------------------------------------------------------------------------
//EOF
