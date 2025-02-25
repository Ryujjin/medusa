/*********************************************************************
 *<
	FILE: SetKeyMode.h

	DESCRIPTION: Interface to set-key mode related APIs

	CREATED BY:	Rolf Berteig

	HISTORY: Created November 29, 2000

 *>	Copyright (c) 2000, All Rights Reserved.
 **********************************************************************/

#ifndef __SETKEYMODE_H__
#define __SETKEYMODE_H__

#define I_SETKEYMODE  0x00002000

//#define SET_KEY_MODE		1<<0
//#define USE_OTHER_FILTERS					(1<<1)
#define		SETKEY_IK_PARAMS				(1<<1)
#define		SETKEY_POS						(1<<2)
#define		SETKEY_ROT						(1<<3)
#define		SETKEY_SCALE					(1<<4)
#define		SETKEY_MODIFIER					(1<<5)
#define		SETKEY_MATERIAL					(1<<6)
#define		SETKEY_ATTRIBUTES				(1<<7)
#define		SETKEY_SELECTED_NODES			(1<<8)
#define		SETKEY_OBJPARAMS				(1<<9)
#define		TIMESLIDER_RBUTTON_DOWN			(1<<10)

#define		HOTKEY_ALL_POSROTSCALE			(1<<11)

#define		HOTKEY_ALL_POS					(1<<12)
#define		HOTKEY_ALL_POSX					(1<<13)
#define		HOTKEY_ALL_POSY					(1<<14)
#define		HOTKEY_ALL_POSZ					(1<<15)

#define		HOTKEY_ALL_ROT					(1<<16)
#define		HOTKEY_ALL_ROTX					(1<<17)
#define		HOTKEY_ALL_ROTY					(1<<18)
#define		HOTKEY_ALL_ROTZ					(1<<19)

#define		HOTKEY_ALL_SCALE				(1<<20)
#define		HOTKEY_ALL_SCALEX				(1<<21)
#define		HOTKEY_ALL_SCALEY				(1<<22)
#define		HOTKEY_ALL_SCALEZ				(1<<23)


// Gets a pointer to the SetKeyModeInterface interface, the caller should pass a pointer to "Interface"
#define GetSetKeyModeInterface(i)  ((SetKeyModeInterface*)i->GetInterface(I_SETKEYMODE))


class SetKeyModeCallback : public InterfaceServer {
	public:
		virtual void SetKey()=0;				// Called when user executes set key command
		virtual void ShowUI()=0;				// Display set key floater window
		virtual void SetKeyModeStateChanged()=0;// Called when user enters or exits set-key mode
	};

class SetKeyModeInterface : public InterfaceServer {
	public:
		virtual void RegisterSetKeyModeCallback(SetKeyModeCallback *cb)=0;
		virtual void UnRegisterSetKeyModeCallback(SetKeyModeCallback *cb)=0;
		
		virtual void ActivateSetKeyMode(BOOL onOff)=0;
		virtual void AllTracksCommitSetKeyBuffer()=0;
		virtual void AllTracksRevertSetKeyBuffer()=0;
		virtual BOOL AllTracksSetKeyBufferPresent()=0;
		
		virtual void SetSKFlag(DWORD mask) =0; 
		virtual void ClearSKFlag(DWORD mask) =0; 
		virtual BOOL TestSKFlag(DWORD mask) =0; 

		virtual BOOL CreateTransformKeysIfBuffersNotPresent(TimeValue, INode *, BOOL ) =0;
		virtual void SetKeyCommit(void ) =0;
		virtual void RecursiveSetKeyCommit(const TimeValue &t,Animatable *anim, const TSTR &characterSetName, UINT recursiveFlags = FALSE) =0;
		virtual void CommitSetKeys(const BOOL &selectOveride = FALSE) =0;
	};

#endif //__SETKEYMODE_H__


