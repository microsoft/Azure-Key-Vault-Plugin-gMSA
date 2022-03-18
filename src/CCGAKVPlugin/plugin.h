//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//

#pragma once

typedef struct _KerbInteractiveLogonHelper
{
    std::vector<char> m_buffer;  // Uses non-exception throwing variant because this stack doesn't catch exceptions

    _KerbInteractiveLogonHelper();
    HRESULT InsertLogonInformation(const std::wstring domain, const std::wstring user, const std::wstring password);
    PKERB_INTERACTIVE_LOGON Buffer() const;
    ULONG32 Size() const;
    HRESULT InsertUnicodeString(std::wstring str, UNICODE_STRING* target);
    HRESULT GetCOMCopy(BYTE** buffer) const;

} KerbInteractiveLogonHelper;
