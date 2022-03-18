//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
#include "pch.h"
#include <wrl\module.h>
#include "winstring.h"

using namespace Microsoft::WRL;

// To stop the compiler from optimizing out the COM class factory
CoCreatableClassWrlCreatorMapInclude(CCGPlugin);

#if !defined(__WRL_CLASSIC_COM__)
STDAPI DllGetActivationFactory(_In_ HSTRING activatibleClassId, _COM_Outptr_ IActivationFactory** factory)
{
    return Module<InProc>::GetModule().GetActivationFactory(activatibleClassId, factory);
}
#endif

#if !defined(__WRL_WINRT_STRICT__)
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppv)
{
    return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}
#endif

STDAPI DllCanUnloadNow() { return Module<InProc>::GetModule().Terminate() ? S_OK : S_FALSE; }

