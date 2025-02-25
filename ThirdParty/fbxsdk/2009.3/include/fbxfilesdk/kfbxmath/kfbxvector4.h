/*!  \file kfbxvector4.h
 */
 
#ifndef FBXFILESDK_KFBXMATH_KFBXVECTOR4_H
#define FBXFILESDK_KFBXMATH_KFBXVECTOR4_H

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
#include <fbxfilesdk/components/kbaselib/klib/karrayul.h>

#include <fbxfilesdk/kfbxplugins/kfbxtypes.h>

#include <fbxfilesdk/fbxfilesdk_nsbegin.h>
class KFbxQuaternion;
	/**	FBX SDK 4-elements vector class.
  * \nosubgrouping
  */
	class KFBX_DLL KFbxVector4 : public fbxDouble4
	{

	public:

	/**
	  * \name Constructors and Destructor
	  */
	//@{

	//! Constructor.
	KFbxVector4();

	/** Copy constructor.
	  * \param pVector4   The vector copied to this one.
	  */
	KFbxVector4(const KFbxVector4& pVector4);

	/** Constructor.
	  *	\param pX X component.
	  *	\param pY Y component.
	  *	\param pZ Z component.
	  *	\param pW W component.
	  */
	KFbxVector4(double pX, double pY, double pZ, double pW = 1.0);

	/** Constructor.
	  *	\param pValue X,Y,Z,W components.
	  */
	KFbxVector4(const double pValue[4]);

	/** Constructor.
	  * \param pValue X,Y,Z components.
      * \remarks The fourth component of this object is assigned 1.
	  */
	KFbxVector4(const fbxDouble3& pValue);

	//! Destructor.
	~KFbxVector4();

	//@}

	/**
	  * \name Access
	  */
	//@{
	
	/** Assignment operation.
	  * \param pVector4  The vector assigned to this one.
	  * \return          This vector after assignment.
	  */
	KFbxVector4& operator=(const KFbxVector4& pVector4);

    /** Assignment operation.
	  * \param pValue    The pointer to an array whose elements are asigned to this vector.
	  * \return          This vector after assignment.
	  */
	KFbxVector4& operator=(const double* pValue);
	
    /** Assignment operation.
	  * \param pValue    The vector with 3 elements assigned to this vector.
	  * \return          This vector after assignment.
	  * \remarks         The first three elements are assigned with pValue. The fourth element is set as 1.0 
	  */
	KFbxVector4& operator=(const fbxDouble3& pValue);

	/** Accessor.
	  * \param pIndex The index of the component to access.
	  * \return The reference to the indexed component.
	  * \remarks          The parameter is not checked for values out of bounds. The valid range is [0,3].
	  */
	double& operator[](int pIndex);

	/** Accessor.
	  * \param pIndex The index of the component to access.
	  * \return The reference to the indexed component.
	  * \remarks          The parameter is not checked for values out of bounds. The valid range is [0,3].
	  */
    double const& operator[](int pIndex) const;

	/** Get a vector element.
	  * \param pIndex The index of the component to access.
	  * \return The value of the indexed component.
	  * \remarks          The parameter is not checked for values out of bounds. The valid range is [0,3].
	  */
	double GetAt(int pIndex);	

	/** Set a vector element.
      * \param pIndex The index of the component to set.
	  * \param pValue The new value to set the component.
	  * \remarks          The index parameter is not checked for values out of bounds. The valid range is [0,3].
	  */
	void SetAt(int pIndex, double pValue);

	/** Set vector.
	  * \param pX The X component value.
  	  * \param pY The Y component value.
	  * \param pZ The Z component value.
	  * \param pW The W component value.
	  */
	void Set(double pX, double pY, double pZ, double pW = 1.0);
	
	//@}

	/**
	  * \name Scalar Operations
	  */
	//@{

	/** Add a value to all vector components.
	  * \param pValue The value to add to each component of the vector.
	  * \return New vector.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4 operator+(double pValue);

	/** Substract a value from all vector components.
	  * \param pValue The value to substract from each component of the vector.
	  * \return New vector.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4 operator-(double pValue);

	/** Multiply a value to all vector components.
	  * \param pValue The value multiplying each component of the vector.
	  * \return New vector.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4 operator*(double pValue);

	/**	Divide all vector components by a value.
	  * \param pValue The value dividing each component of the vector.
	  * \return New vector.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4 operator/(double pValue);

	/** Add a value to all vector components.
	  * \param pValue The value to add to each component of the vector.
	  * \return \e this updated with the operation result.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4& operator+=(double pValue);

	/** Subtract a value from all vector components.
	  * \param pValue The value to subtract from each component of the vector.
	  * \return \e this updated with the operation result.
  	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4& operator-=(double pValue);

	/** Multiply a value to all vector elements.
	  * \param pValue The value multiplying each component of the vector.
	  * \return \e this updated with the operation result.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4& operator*=(double pValue);

	/**	Divide all vector elements by a value.
	  * \param pValue The value dividing each component of the vector.
	  * \return \e this updated with the operation result.
	  * \remarks          The passed value is not checked.
	  */
	KFbxVector4& operator/=(double pValue);
	
	//@}

	/**
	  * \name Vector Operations
	  */
	//@{

	/**	Unary minus operator.
	  * \return The vector that is the negation of \c this.
	  */
	KFbxVector4 operator-();
	
	/** Add two vectors together.
	  * \param pVector Vector to add.
	  * \return The vector v' = this + pVector.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4 operator+(KFbxVector4& pVector);

	/** Subtract a vector from another vector.
	  * \param pVector Vector to subtract.
	  * \return The vector v' = this - pVector.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4 operator-(KFbxVector4& pVector);

	/** Memberwise multiplication of two vectors.
	  * \param pVector Multiplying vector.
	  * \return The vector v' = this * pVector.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4 operator*(KFbxVector4& pVector);

	/** Memberwise division of a vector with another vector.
	  * \param pVector Dividing vector.
	  * \return The vector v[i]' = this[i] / pVector[i].
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4 operator/(KFbxVector4& pVector);

	/** Add two vectors together.
	  * \param pVector Vector to add.
	  * \return \e this updated with the operation result.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4& operator+=(KFbxVector4& pVector);

	/** Subtract a vector from another vector.
	  * \param pVector Vector to subtract.
	  * \return \e this updated with the operation result.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4& operator-=(KFbxVector4& pVector);

	/** Memberwise multiplication of two vectors.
	  * \param pVector Multiplying vector.
	  * \return \e this updated with the operation result.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4& operator*=(KFbxVector4& pVector);
	
	/** Memberwise division of a vector with another vector.
	  * \param pVector Dividing vector.
	  * \return \e this updated with the operation result.
	  * \remarks           The values in pVector are not checked.
	  */
	KFbxVector4& operator/=(KFbxVector4& pVector);

	/** Calculate the dot product of two vectors.
	  * \param pVector The second vector.
	  * \return The dot product value.
      * \remarks           Being considered as a XYZ vector with a weight, only the 3 first elements are considered in this operation.
	  */
	double DotProduct(KFbxVector4& pVector);

	/** Calculate the cross product of two vectors.
	  * \param pVector The second vector.
	  * \return The cross product vector.
      * \remarks           Being considered as a XYZ vector with a weight, only the first 3 elements are considered in this operation.
	  */
	KFbxVector4 CrossProduct(KFbxVector4& pVector);

	/** Calculate the Euler rotation required to align axis pAB-pA on pAB-pB.
	  *	\param pAB The intersection of the 2 axis.
	  *	\param pA A point on axis to be aligned.
	  *	\param pB A point on reference axis.
	  *	\param pAngles Resulting euler angles.
	  *	\return \c true on success.
	  * \remarks           Being considered as a XYZ vector with a weight, only the first 3 elements are considered in this operation.
	  */
	static bool AxisAlignmentInEulerAngle(KFbxVector4 const& pAB, 
		                                  KFbxVector4 const& pA, 
										  KFbxVector4 const& pB, 
										  KFbxVector4& pAngles);
	
	//@}

	/**
	  * \name Boolean Operations
	  */
	//@{

	/**	Equivalence operator.
	  * \param pVector The vector to be compared to \e this.
	  * \return            \c true if the two vectors are equal (each element is within a 1.0e-6 tolerance) and \c false otherwise.
	  */
	bool operator==(KFbxVector4& pVector);

	/**	Equivalence operator.
	  * \param pVector The vector to be compared to \e this.
	  * \return            \c true if the two vectors are equal (each element is within a 1.0e-6 tolerance) and \c false otherwise.
	  */
	bool operator==(KFbxVector4 const& pVector) const;
	
	/**	Non equivalence operator.
	  * \param pVector The vector to be compared to \e this.
	  * \return            \c false if the two vectors are equal (each element is within a 1.0e-6 tolerance) and \c true otherwise.
	  */
	bool operator!=(KFbxVector4& pVector);

	/**	Non equivalence operator.
	  * \param pVector The vector to be compared to \e this.
	  * \return            \c false if the two vectors are equal (each element is within a 1.0e-6 tolerance) and \c true otherwise.
	  */
	bool operator!=(KFbxVector4 const& pVector) const;
	
	//@}

	/**
	  * \name Length
	  */
	//@{

	/** Get the vector's length.
	  * \return The mathematical length of the vector.
	  * \remarks     Being considered as a XYZ vector with a weight, only the first 3 elements are considered in this operation.
	  */
	double Length();

	/** Get the vector's length squared.
	  * \return The mathematical square length of the vector.
	  * \remarks     Being considered as a XYZ vector with a weight, only the first 3 elements are considered in this operation.
	  */
	double SquareLength();

	/** Find the distance between 2 vectors.
	  * \param pVector The second vector.
	  * \return The mathematical distance between the two vectors.
	  * \remarks           Being considered as a XYZ vector with a weight, only the 3 first elements are considered in this operation.
	  */
	double Distance(KFbxVector4& pVector);

	/** Normalize the vector, length set to 1.
	  * \remarks     Being considered as a XYZ vector with a weight, only the first 3 elements are considered in this operation.
	  */
	void Normalize();


    /** Set the Euler XYZ from a Quaternion.
	  *\param pQuat    Quaternion from which Euler XYZ information is got.
      */
    void SetXYZ(KFbxQuaternion pQuat);

	//@}

	/**
	  * \name Casting
	  */
	//@{

	//! Cast the vector in a double pointer.
	operator double* ();

	//! Cast the vector in a const double pointer.
	operator const double* ();

	//! Cast the vector in a const double pointer.
	operator const double* () const;
	

	//@}

	///////////////////////////////////////////////////////////////////////////////
	//
	//  WARNING!
	//
	//	Anything beyond these lines may not be documented accurately and is 
	// 	subject to change without notice.
	//
	///////////////////////////////////////////////////////////////////////////////

	#ifndef DOXYGEN_SHOULD_SKIP_THIS	



	#endif // #ifndef DOXYGEN_SHOULD_SKIP_THIS

	};

	typedef  class KFBX_DLL KArrayTemplate<KFbxVector4>				KArrayKFbxVector4;
	typedef KFbxVector4* HKFbxVector4;

	inline EFbxType FbxTypeOf( KFbxVector4	const &pItem )	{ return eDOUBLE4; }


#include <fbxfilesdk/fbxfilesdk_nsend.h>

#endif // FBXFILESDK_KFBXMATH_KFBXVECTOR4_H

