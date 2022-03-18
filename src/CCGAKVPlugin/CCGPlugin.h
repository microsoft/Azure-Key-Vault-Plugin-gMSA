//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for CCGPlugin.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __CCGPlugin_h__
#define __CCGPlugin_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICcgDomainAuthCredentials_FWD_DEFINED__
#define __ICcgDomainAuthCredentials_FWD_DEFINED__
typedef interface ICcgDomainAuthCredentials ICcgDomainAuthCredentials;

#endif 	/* __ICcgDomainAuthCredentials_FWD_DEFINED__ */


#ifndef __CCGPlugin_FWD_DEFINED__
#define __CCGPlugin_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCGPlugin CCGPlugin;
#else
typedef struct CCGPlugin CCGPlugin;
#endif /* __cplusplus */

#endif 	/* __CCGPlugin_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __ICcgDomainAuthCredentials_INTERFACE_DEFINED__
#define __ICcgDomainAuthCredentials_INTERFACE_DEFINED__

/* interface ICcgDomainAuthCredentials */
/* [unique][uuid][object] */ 


EXTERN_C const IID IID_ICcgDomainAuthCredentials;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6ecda518-2010-4437-8bc3-46e752b7b172")
    ICcgDomainAuthCredentials : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPasswordCredentials( 
            /* [in] */ LPCWSTR PluginInput,
            /* [out] */ LPWSTR *domainName,
            /* [out] */ LPWSTR *username,
            /* [out] */ LPWSTR *password) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct ICcgDomainAuthCredentialsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICcgDomainAuthCredentials * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICcgDomainAuthCredentials * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICcgDomainAuthCredentials * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPasswordCredentials )( 
            ICcgDomainAuthCredentials * This,
            /* [in] */ LPCWSTR PluginInput,
            /* [out] */ LPWSTR *domainName,
            /* [out] */ LPWSTR *username,
            /* [out] */ LPWSTR *password);
        
        END_INTERFACE
    } ICcgDomainAuthCredentialsVtbl;

    interface ICcgDomainAuthCredentials
    {
        CONST_VTBL struct ICcgDomainAuthCredentialsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICcgDomainAuthCredentials_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICcgDomainAuthCredentials_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICcgDomainAuthCredentials_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICcgDomainAuthCredentials_GetPasswordCredentials(This,PluginInput,domainName,username,password)	\
    ( (This)->lpVtbl -> GetPasswordCredentials(This,PluginInput,domainName,username,password) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICcgDomainAuthCredentials_INTERFACE_DEFINED__ */



#ifndef __CCGPluginLib_LIBRARY_DEFINED__
#define __CCGPluginLib_LIBRARY_DEFINED__

/* library CCGPluginLib */
/* [version][uuid] */ 


EXTERN_C const IID LIBID_CCGPluginLib;

EXTERN_C const CLSID CLSID_CCGPlugin;

#ifdef __cplusplus

class DECLSPEC_UUID("ccc2a336-d7f3-4818-a213-272b7924213e")
CCGPlugin;
#endif
#endif /* __CCGPluginLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


