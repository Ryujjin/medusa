/**
	@file ImportFBX.h
	@brief TODO

	(c)2009 Palestar Inc
	@author Richard Lyle @date 10/19/2009 1:23:48 PM
*/

#ifndef IMPORTDAE_H
#define IMPORTDAE_H

#include "Tools/ScenePort/Port.h"

#include <map>
#include <string>

struct aiScene;
struct aiNode;
struct aiMaterial;

//---------------------------------------------------------------------------------------------------

class ImportFBX : public ScenePort::Importer
{
public:
	//! Construction
	ImportFBX();
	~ImportFBX();

	//! ScenePort::Importer interface
	bool				import( ScenePort * a_pScene, const char * a_pFileName );

protected:
	//! Data
	CharString			m_sImportFile;			// filename of file being imported
	CharString			m_sBaseDirectory;		// directory that contains the file we are importing

	const aiScene *			m_pScene;		// scene being imported, valid only during import()
	
	//! Mutators
	bool				importNodes( ScenePort * a_pScene, 
							BaseNodePort * a_pParent, 
							const aiNode * a_pNode );

	NodePort *			importNode( ScenePort * a_pScene,
							BaseNodePort * a_pParent,
							const aiNode * a_pNode );
	NodePort *			importMesh( ScenePort * a_pScene,
							BaseNodePort * a_pParent,
							const aiNode * a_pNode );
	CharString			importMaterial( ScenePort * a_pScene, 
							const aiMaterial * a_pMaterial, const char * a_pDefaultName );
	CharString			importImage( ScenePort * a_pScene, 
							const char * a_pFile );
};

#endif

//---------------------------------------------------------------------------------------------------
//EOF
