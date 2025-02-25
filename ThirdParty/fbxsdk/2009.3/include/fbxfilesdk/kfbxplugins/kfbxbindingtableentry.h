#ifndef FBXFILESDK_KFBXPLUGINS_KFBXBINDINGTABLEENTRY_H 
#define FBXFILESDK_KFBXPLUGINS_KFBXBINDINGTABLEENTRY_H

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

#include <fbxfilesdk/components/kbaselib/kaydaradef_h.h>
#include <fbxfilesdk/components/kbaselib/kaydara.h>

#include <fbxfilesdk/kfbxplugins/kfbxobject.h>

#include <fbxfilesdk/fbxfilesdk_nsbegin.h>

// Forward declarations
class KFbxSdkManager;
class KFbxBindingTable;

/** A binding table entry stores a binding between a source and a 
  * destination. Users should not instiantiate this class directly!
  * \nosubgrouping
  */
class KFBX_DLL KFbxBindingTableEntry 
{

public:
	/** 
	  *\name Constructor and Destructor
	  */
	//@{
	//!Constructor.
	KFbxBindingTableEntry();
	/**Copy constructor.
	  * \param pEntry KFbxBindingTableEntry to be copied. 
	  */
	KFbxBindingTableEntry(KFbxBindingTableEntry const& pEntry);
	//!Destructor.
	~KFbxBindingTableEntry();
	//@}

    /**
	  * \name Access
	  */
	//@{

	/** Set the source. 
	  * \param pSource             The source to set.
	  */
	void SetSource( const char* pSource );

	//!Retrieve the source.
	const char* GetSource() const;

    /** Set the destination. 
	  * \param pDestination             The destination to set.
	  */
	void SetDestination( const char* pDestination );

	//!Retrieve the destination.
	const char* GetDestination() const;
    
	/** Set the source type or destination type. 
	  * \param pType             The source type or destination type to set.
	  * \param pAsSource         Flag indicates soure type or destination type to set.
	  */
	void SetEntryType( const char* pType, bool pAsSource );
   
    /** Get the source type or destination type. 
	  * \param pAsSource         Flag indicates soure type or destination type to get.
	  * \return                  Source type or destination type.
	  */
	const char* GetEntryType( bool pAsSource ) const;


	//!Retrieve user data pointer.	  
	void* GetUserDataPtr();
	//!Retrieve user data pointer.
	const void* GetUserDataPtr() const;
	/**Set user data pointer.
	  * \param pData user data pointer. 
	  */
	void SetUserDataPtr( void* pData );
	//@}
   
	/**Assignment operator.
	  * \param pEntry KFbxBindingTableEntry assigned to this one.
	  */
	KFbxBindingTableEntry& operator=(KFbxBindingTableEntry const& pEntry);


///////////////////////////////////////////////////////////////////////////////
//
//  WARNING!
//
//	Anything beyond these lines may not be documented accurately and is 
// 	subject to change without notice.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef DOXYGEN_SHOULD_SKIP_THIS

protected:
	KString mSource;
	KString mDestination;

	KString mSourceType;
	KString mDestinationType;

	void* mData;
private:

#endif // #ifndef DOXYGEN_SHOULD_SKIP_THIS

};

#include <fbxfilesdk/fbxfilesdk_nsend.h>

#endif // FBXFILESDK_KFBXPLUGINS_KFBXBINDINGTABLEENTRY_H

