#ifndef FBXFILESDK_KFBXMODULES_KFBXLOADINGSTRATEGY_H
#define FBXFILESDK_KFBXMODULES_KFBXLOADINGSTRATEGY_H

/**************************************************************************************

 Copyright ?2001 - 2006 Autodesk, Inc. and/or its licensors.
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
// Local includes 
#include <fbxfilesdk/kfbxmodules/kfbxplugin.h>
#include <fbxfilesdk/kfbxmodules/kfbxplugincontainer.h>

// FBX begin namespace
#include <fbxfilesdk/fbxfilesdk_nsbegin.h>

/////////////////////////////////////////////////////////////////////////////////////////
// TODO: todonamespace
//       Every FBX module should have a namespace but for consistency, 
//       you have, for this version of fbx, to use the fbx object without namespace
//namespace kfbxmodules
//{
    /** \class KFbxLoadingStrategy
    *
    * \brief Abstract class used to implemented some plug-in loading strategy.
    *        A loading strategy dictate how some plug-ins will be loaded for instance
    *        we could have a simple strategy that loads only a single dll on PC. We
    *        could also implement a strategy that load multiple dlls from a directory. 
    * 
    */
    class KFBX_DLL KFbxLoadingStrategy : public KFbxPluginContainer
    {

    public:
        /**
        *\name Public interface
        */
        //@{

        /** Execute the operation of loading the plug-in(s). The way it is executed is determined by the specific implementations.
        * \param pData  Plug in data that can be access inside the plug-ins
        * \return If the plugin loading is successful return \c true, otherwise return \c false
        */
        bool Load(KFbxPluginData& pData);

        /** Execute the operation of unloading the plug-in(s). The way it is executed is determined by the specific implementations.
        */
        void Unload();
        //@}

    protected:
        /**
        *\name User implementation
        */
        //@{
        /** Called by the Load method, it contains the specific user implementation strategy to load the desired plug-in(s)
		* \param pData  Plug in data that can be access inside the plug-ins.
        * \return If the plugin loading is successful return \c true, otherwise return \c false
        */
        virtual bool SpecificLoad(KFbxPluginData& pData) = 0;

        /** Called by the Unload method, it contains the specific user implementation strategy to unload the desired plug-in(s)
        */
        virtual void SpecificUnload() = 0;
        //@}

        /** Whether the plugin is loaded or not
         */
        bool mIsLoaded;

    private:
        KFbxPluginData mData;
    };
//}

// FBX end namespace
#include <fbxfilesdk/fbxfilesdk_nsend.h>

#endif // FBXFILESDK_KFBXMODULES_KFBXLOADINGSTRATEGY_H

