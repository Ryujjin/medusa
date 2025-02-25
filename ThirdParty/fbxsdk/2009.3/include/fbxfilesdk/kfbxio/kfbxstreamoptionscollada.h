/*!  \file kfbxstreamoptionscollada.h
 */
 
#ifndef FBXFILESDK_KFBXIO_KFBXSTREAMOPTIONSCOLLADA_H
#define FBXFILESDK_KFBXIO_KFBXSTREAMOPTIONSCOLLADA_H

/**************************************************************************************

 Copyright � 2001 - 2008 Autodesk, Inc. and/or its licensors.
 All Rights Reserved.

 The coded instructions, statements, computer programs, and/or related material 
 (collectively the "Data") in these files contain unpublished information 
 proprietary to Autodesk, Inc. and/or its licensors, which is protected by 
 Canada and United States of America federal copyright law and by international 
 treaties. 
 
 The Data may not be disclosed or distributed to third parties, in whole or in
 part, without the prior written consent of Autodesk, Inc. ("Autodesk").

 THE DATA IS PROVIDED "AS IS" AND WITHOUT WARRANTY.
 ALL WARRANTIES ARE EXPRESSLY EXCLUDED AND DISCLAIMED. AUTODESK MAKES NO
 WARRANTY OF ANY KIND WITH RESPECT TO THE DATA, EXPRESS, IMPLIED OR ARISING
 BY CUSTOM OR TRADE USAGE, AND DISCLAIMS ANY IMPLIED WARRANTIES OF TITLE, 
 NON-INFRINGEMENT, MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE OR USE. 
 WITHOUT LIMITING THE FOREGOING, AUTODESK DOES NOT WARRANT THAT THE OPERATION
 OF THE DATA WILL BE UNINTERRUPTED OR ERROR FREE. 
 
 IN NO EVENT SHALL AUTODESK, ITS AFFILIATES, PARENT COMPANIES, LICENSORS
 OR SUPPLIERS ("AUTODESK GROUP") BE LIABLE FOR ANY LOSSES, DAMAGES OR EXPENSES
 OF ANY KIND (INCLUDING WITHOUT LIMITATION PUNITIVE OR MULTIPLE DAMAGES OR OTHER
 SPECIAL, DIRECT, INDIRECT, EXEMPLARY, INCIDENTAL, LOSS OF PROFITS, REVENUE
 OR DATA, COST OF COVER OR CONSEQUENTIAL LOSSES OR DAMAGES OF ANY KIND),
 HOWEVER CAUSED, AND REGARDLESS OF THE THEORY OF LIABILITY, WHETHER DERIVED
 FROM CONTRACT, TORT (INCLUDING, BUT NOT LIMITED TO, NEGLIGENCE), OR OTHERWISE,
 ARISING OUT OF OR RELATING TO THE DATA OR ITS USE OR ANY OTHER PERFORMANCE,
 WHETHER OR NOT AUTODESK HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH LOSS
 OR DAMAGE. 

**************************************************************************************/

#define KFBXSTREAMOPT_COLLADA_TRIANGULATE "TRIANGULATE"
#define KFBXSTREAMOPT_COLLADA_SINGLEMATRIX "SINGLE MATRIX"
#define KFBXSTREAMOPT_COLLADA_FRAME_COUNT "FRAME COUNT"
#define KFBXSTREAMOPT_COLLADA_FRAME_RATE "FRAME RATE"
#define KFBXSTREAMOPT_COLLADA_START "START"
#define KFBXSTREAMOPT_COLLADA_TAKE_NAME "TAKE NAME"



#include <fbxfilesdk/components/kbaselib/kaydaradef_h.h>
#include <fbxfilesdk/kfbxplugins/kfbxsdkmanager.h>
#include <fbxfilesdk/kfbxio/kfbxstreamoptions.h>
#include <fbxfilesdk/fbxfilesdk_nsbegin.h>

/**	\brief This class is used for accessing the Import options of Collada files.
  * The content of KfbxStreamOptionsCollada is stored in the inherited Property of its parent (KFbxStreamOptions).
  */
class KFBX_DLL KFbxStreamOptionsColladaReader : public KFbxStreamOptions
{
	KFBXOBJECT_DECLARE(KFbxStreamOptionsColladaReader,KFbxStreamOptions);

public:

	/** Reset all options to default values
	*/
	void Reset();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
public:
	virtual KFbxObject* Clone(KFbxObject* pContainer, KFbxObject::ECloneType pCloneType) const;

protected:
	KFbxStreamOptionsColladaReader(KFbxSdkManager& pManager, char const* pName);
	virtual ~KFbxStreamOptionsColladaReader();
	virtual void Construct(const KFbxStreamOptionsColladaReader* pFrom);
	virtual bool ConstructProperties( bool pForceSet );
	virtual void Destruct(bool pRecursive, bool pDependents);

#endif
};

/**	\brief This class is used for accessing the Export options of Collada files.
  * The content of KfbxStreamOptionsCollada is stored in the inherited Property of its parent (KFbxStreamOptions).
  */
class KFBX_DLL KFbxStreamOptionsColladaWriter : public KFbxStreamOptions
{
	KFBXOBJECT_DECLARE(KFbxStreamOptionsColladaWriter,KFbxStreamOptions);

public:

	/** Reset all the options to default value
	*/
	void Reset();

	/** Sets the Time Mode
	* \param pTimeMode            The time mode to be used.
	* \param pCustomFrameRate     The value of the frame rate. 
	*/
	void SetTimeMode(KTime::ETimeMode pTimeMode, double pCustomFrameRate = 0.0);

	
	/** Get the Frame Period
	* \return     KTime of the Frame Period
	*/
	KTime GetFramePeriod();

#ifndef DOXYGEN_SHOULD_SKIP_THIS
public:
	virtual KFbxObject* Clone(KFbxObject* pContainer, KFbxObject::ECloneType pCloneType) const;
protected:

	KFbxStreamOptionsColladaWriter(KFbxSdkManager& pManager, char const* pName);
	virtual ~KFbxStreamOptionsColladaWriter();
	virtual void Construct(const KFbxStreamOptionsColladaWriter* pFrom);
	virtual bool ConstructProperties( bool pForceSet );
	virtual void Destruct(bool pRecursive, bool pDependents);
private:
	KTime::ETimeMode mTimeMode;

#endif
};

#include <fbxfilesdk/fbxfilesdk_nsend.h>

#endif // FBXFILESDK_KFBXIO_KFBXSTREAMOPTIONSCOLLADA_H

