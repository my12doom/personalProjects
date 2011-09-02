

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Fri Sep 02 21:39:59 2011
 */
/* Compiler settings for .\phpcrypt.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __phpcrypt_i_h__
#define __phpcrypt_i_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __Icrypt_FWD_DEFINED__
#define __Icrypt_FWD_DEFINED__
typedef interface Icrypt Icrypt;
#endif 	/* __Icrypt_FWD_DEFINED__ */


#ifndef __crypt_FWD_DEFINED__
#define __crypt_FWD_DEFINED__

#ifdef __cplusplus
typedef class crypt crypt;
#else
typedef struct crypt crypt;
#endif /* __cplusplus */

#endif 	/* __crypt_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __Icrypt_INTERFACE_DEFINED__
#define __Icrypt_INTERFACE_DEFINED__

/* interface Icrypt */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_Icrypt;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C2B9965B-1935-4122-BC33-B3BFC21A1703")
    Icrypt : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE test( 
            BSTR *ret) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE test2( 
            /* [retval][out] */ BSTR *rtn) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_passkey( 
            BSTR input,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_hash( 
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *rtn) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE get_key( 
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE decode_message( 
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE AES( 
            BSTR data,
            BSTR key,
            /* [retval][out] */ BSTR *ret) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE gen_key( void) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE gen_keys( 
            BSTR passkey,
            DATE time_start,
            DATE time_end,
            /* [retval][out] */ BSTR *out) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE gen_keys_int( 
            BSTR passkey,
            ULONG time_start,
            ULONG time_end,
            /* [retval][out] */ BSTR *out) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE genkeys( 
            BSTR passkey,
            LONG time_start,
            LONG time_end,
            /* [retval][out] */ BSTR *out) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE decode_binarystring( 
            BSTR in,
            /* [retval][out] */ BSTR *out) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SHA1( 
            BSTR in,
            /* [retval][out] */ BSTR *out) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE genkeys2( 
            BSTR passkey,
            LONG time_start,
            LONG time_end,
            LONG max_bar_user,
            /* [retval][out] */ BSTR *out) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IcryptVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            Icrypt * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            Icrypt * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            Icrypt * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            Icrypt * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            Icrypt * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            Icrypt * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            Icrypt * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *test )( 
            Icrypt * This,
            BSTR *ret);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *test2 )( 
            Icrypt * This,
            /* [retval][out] */ BSTR *rtn);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *get_passkey )( 
            Icrypt * This,
            BSTR input,
            /* [retval][out] */ BSTR *ret);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *get_hash )( 
            Icrypt * This,
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *rtn);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *get_key )( 
            Icrypt * This,
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *ret);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *decode_message )( 
            Icrypt * This,
            /* [in] */ BSTR input,
            /* [retval][out] */ BSTR *ret);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *AES )( 
            Icrypt * This,
            BSTR data,
            BSTR key,
            /* [retval][out] */ BSTR *ret);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *gen_key )( 
            Icrypt * This);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *gen_keys )( 
            Icrypt * This,
            BSTR passkey,
            DATE time_start,
            DATE time_end,
            /* [retval][out] */ BSTR *out);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *gen_keys_int )( 
            Icrypt * This,
            BSTR passkey,
            ULONG time_start,
            ULONG time_end,
            /* [retval][out] */ BSTR *out);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *genkeys )( 
            Icrypt * This,
            BSTR passkey,
            LONG time_start,
            LONG time_end,
            /* [retval][out] */ BSTR *out);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *decode_binarystring )( 
            Icrypt * This,
            BSTR in,
            /* [retval][out] */ BSTR *out);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *SHA1 )( 
            Icrypt * This,
            BSTR in,
            /* [retval][out] */ BSTR *out);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *genkeys2 )( 
            Icrypt * This,
            BSTR passkey,
            LONG time_start,
            LONG time_end,
            LONG max_bar_user,
            /* [retval][out] */ BSTR *out);
        
        END_INTERFACE
    } IcryptVtbl;

    interface Icrypt
    {
        CONST_VTBL struct IcryptVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define Icrypt_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define Icrypt_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define Icrypt_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define Icrypt_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define Icrypt_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define Icrypt_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define Icrypt_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define Icrypt_test(This,ret)	\
    ( (This)->lpVtbl -> test(This,ret) ) 

#define Icrypt_test2(This,rtn)	\
    ( (This)->lpVtbl -> test2(This,rtn) ) 

#define Icrypt_get_passkey(This,input,ret)	\
    ( (This)->lpVtbl -> get_passkey(This,input,ret) ) 

#define Icrypt_get_hash(This,input,rtn)	\
    ( (This)->lpVtbl -> get_hash(This,input,rtn) ) 

#define Icrypt_get_key(This,input,ret)	\
    ( (This)->lpVtbl -> get_key(This,input,ret) ) 

#define Icrypt_decode_message(This,input,ret)	\
    ( (This)->lpVtbl -> decode_message(This,input,ret) ) 

#define Icrypt_AES(This,data,key,ret)	\
    ( (This)->lpVtbl -> AES(This,data,key,ret) ) 

#define Icrypt_gen_key(This)	\
    ( (This)->lpVtbl -> gen_key(This) ) 

#define Icrypt_gen_keys(This,passkey,time_start,time_end,out)	\
    ( (This)->lpVtbl -> gen_keys(This,passkey,time_start,time_end,out) ) 

#define Icrypt_gen_keys_int(This,passkey,time_start,time_end,out)	\
    ( (This)->lpVtbl -> gen_keys_int(This,passkey,time_start,time_end,out) ) 

#define Icrypt_genkeys(This,passkey,time_start,time_end,out)	\
    ( (This)->lpVtbl -> genkeys(This,passkey,time_start,time_end,out) ) 

#define Icrypt_decode_binarystring(This,in,out)	\
    ( (This)->lpVtbl -> decode_binarystring(This,in,out) ) 

#define Icrypt_SHA1(This,in,out)	\
    ( (This)->lpVtbl -> SHA1(This,in,out) ) 

#define Icrypt_genkeys2(This,passkey,time_start,time_end,max_bar_user,out)	\
    ( (This)->lpVtbl -> genkeys2(This,passkey,time_start,time_end,max_bar_user,out) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __Icrypt_INTERFACE_DEFINED__ */



#ifndef __phpcryptLib_LIBRARY_DEFINED__
#define __phpcryptLib_LIBRARY_DEFINED__

/* library phpcryptLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_phpcryptLib;

EXTERN_C const CLSID CLSID_crypt;

#ifdef __cplusplus

class DECLSPEC_UUID("735CAF26-1894-4D00-B5A9-9F94A1BC51CA")
crypt;
#endif
#endif /* __phpcryptLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


