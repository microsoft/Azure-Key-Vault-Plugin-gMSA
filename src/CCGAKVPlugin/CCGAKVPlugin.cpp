//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
#include "pch.h"
#include "CCGAKVPlugin.tmh"
#include "CCGPlugin.h"
#include "windows.h"
///
/// CCGAKVPlugin.cpp
///
/// Implementation of a CCG plugin that retrieves credential from an Azure
/// Key Vault.
///
/// The entry points of this DLL are: GetPasswordCredentials
///
using namespace std;
using namespace Microsoft::WRL;
#pragma comment(lib, "Credui.lib")

DWORD       g_dwAttach = 0;         // Number of times DLL_PROCESS_ATTACH was called

///
/// Returns a structure that cleanups a string when its destructor is called,
/// using the standard WIL resources.
///
/// \param Str        The wstring to be cleaned.
///
/// \return A WIL scope_exit object, that will cleanup the object at its
///     destruction.
///
auto SecureZeroMemoryScopeExit(
    _In_ std::wstring& Str
)
{
    return wil::scope_exit([&Str]
        {
            SecureZeroMemory((void*)Str.c_str(), Str.size() * sizeof(WCHAR));
            Str.clear();
        });
}

///
/// This is a stub that defines the entry point for the DLL application.
/// It is only used to init and cleanup WPP.
///
STDAPI_(BOOL) DllMain(_In_opt_ HINSTANCE hinst, DWORD ul_reason_for_call, _In_opt_ void*)
{
    try {
        switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
            WPP_INIT_TRACING(NULL);
            TraceEvents(
                TRACE_LEVEL_INFORMATION,
                TRACE_DLL,
                "%!FUNC!: Dll Process Attach."
            );
            DisableThreadLibraryCalls(hinst);


            if (ERROR_SUCCESS == EventRegisterMicrosoft_AKSGMSAPlugin())
            {
                TraceEvents(
                    TRACE_LEVEL_INFORMATION,
                    TRACE_DLL,
                    "%!FUNC!: EventRegister succeeded."
                );
                ++g_dwAttach;
               
            }
            else
            {
                TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_DLL,
                    "%!FUNC!: EventRegister failed."
                );
                
            }

            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            break;
        case DLL_PROCESS_DETACH:
            TraceEvents(
                TRACE_LEVEL_INFORMATION,
                TRACE_DLL,
                "%!FUNC!: Dll Process Detach."
            );

            if (Microsoft_AKSGMSAPluginHandle != (REGHANDLE)NULL)
            {
                --g_dwAttach;

                if (g_dwAttach <= 0)
                {
                    EventUnregisterMicrosoft_AKSGMSAPlugin();
                }
            }

            WPP_CLEANUP();
            break;
        }

        return TRUE;
    }
    catch (...)
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: Error in DLL_MAIN."
        );

        return false;
    }
}
///
/// Converts a wstring that only has 8-bit characters, to a string.
///
/// \param Str        The wstring to copy.
///
/// \return A string with the content of the wstring.
///
std::string
AnsiWstringToString(
    _In_ const std::wstring& Str
)
{
    std::string result(Str.length(), 0);

    std::transform(
        Str.begin(),
        Str.end(),
        result.begin(),
        [](WCHAR ch) -> unsigned char {
            return (unsigned char)(ch & 0xFF);
        }
    );

    return std::move(result);
}

///
/// Parses a JSON string that contains the response of the MSI requests and retrieves
/// the service access token.
///
/// \param ResponseBody        The JSON string with the MSI response.
/// \param AccessToken         The output parameter, that will contain the recovered
///     access token.
///
/// \return True if the access token attribute was found in the JSON, and successfully
///     retrieved; Otherwise, returns false.
///
bool
GetAccessTokenFromResponse(
    _In_ std::wstring& objectId,
    _In_ std::wstring& ResponseBody,
    _Inout_ std::wstring& AccessToken
)
{
    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        "%!FUNC!: GetAccessTokenFromResponse called."
    );
   
    JsonFileParser jsonParser(ResponseBody);

    std::wstring tempAccessToken;
    auto zeroAccessToken = SecureZeroMemoryScopeExit(tempAccessToken);

    std::wstring tempTokenType;

    try
    {
        if (jsonParser.BeginParseObject())
        {
            do
            {
                const std::wstring key(jsonParser.GetKey());

                if (_wcsicmp(key.c_str(), L"token_type") == 0)
                {
                    tempTokenType = jsonParser.ParseStringValue();
                    jsonParser.CleanValueBuffer();
                }
                else if (_wcsicmp(key.c_str(), L"access_token") == 0)
                {
                    tempAccessToken = jsonParser.ParseStringValue();
                    jsonParser.CleanValueBuffer();
                }
                else
                {
                    jsonParser.SkipValue();
                }

            } while (jsonParser.ParseNextObjectElement());
        }
    }
    catch (...)
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL, ""
            "%!FUNC!: Error parsing the Json.");

        EventWriteCCG_AKV_PLUGIN_GET_ACCESS_TOKEN_FROM_RESPONSE_JSON_ERROR(objectId.c_str());

        return false;
    }

    if (tempAccessToken.empty())
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: The 'access_token' tag wasn't found.");

        EventWriteCCG_AKV_PLUGIN_GET_ACCESS_TOKEN_FROM_RESPONSE_MISSING_ACCESS_TOKEN(objectId.c_str());
        return false;
    }

    if (tempTokenType.empty())
    {
        tempTokenType = L"Bearer";
    }

    AccessToken = tempTokenType + U(" ") + tempAccessToken;

    return true;
}

///
/// Returns a task with the request to MSI, to obtain the access token.
///
/// \param AccessToken         The output parameter, that will contain the
///     access token.
///
/// \return A task that requests the access token, and will return a status
///     code with the result.
///
HRESULT
RequestAccessToken(
    _In_ std::wstring objectId, 
     _In_ std::wstring resource, 
    _Out_ std::wstring& AccessToken
)
{
    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        "%!FUNC!: RequestAccessToken called. Object ID is  %S.",
        objectId.c_str()
    );

    utility::string_t url = U("http://169.254.169.254/");
    web::http::client::http_client client(url);

    web::http::uri_builder builder(U("/metadata/identity/oauth2/token"));
    builder.append_query(U("api-version"), U("2020-10-01"));

    builder.append_query(U("object_id"), objectId);

    builder.append_query(U("resource"), resource);

    //
    // Instantiate our own response buffer, to be able to clean its memory
    // after using it.
    //
    auto tokenRequestBuffer = std::make_shared< Concurrency::streams::container_buffer<std::vector<uint8_t>> >();

    web::http::http_request request(web::http::methods::GET);
    request.set_request_uri(builder.to_uri());
    request.set_response_stream(tokenRequestBuffer->create_ostream());
    request.headers().add(U("Metadata"), U("true"));

    //std::wcout << L"Requesting access token to:" << (PWCHAR)client.base_uri().to_string().c_str() << (PWCHAR)request.absolute_uri().to_string().c_str() << L"\n";

   
    web::http::http_response response = client.request(request).get();
    HRESULT status = S_OK;

    if (response.status_code() == 200)
    {
        response.content_ready().get();

        std::wstring responseBody;
        auto zeroResponseBody = SecureZeroMemoryScopeExit(responseBody);
        auto zeroBuffer = wil::SecureZeroMemory_scope_exit(
            &(tokenRequestBuffer->collection())[0],
            tokenRequestBuffer->collection().size() * sizeof((tokenRequestBuffer->collection())[0]));

        responseBody.assign(
            &(tokenRequestBuffer->collection())[0],
            &(tokenRequestBuffer->collection())[0] + tokenRequestBuffer->collection().size()
        );

        //
        // Extract the access token from the JSON response.
        //
        bool success = GetAccessTokenFromResponse(objectId, responseBody, AccessToken);

        if (!success)
        {
            status = E_INVALIDARG;
        }
    }
    else if (response.status_code() == 400)
    {
        //
        // The identity with that object id wasn't found.
        //
        status = E_INVALIDARG;

        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: The identity with the given object id wasn't found."
        );

        EventWriteCCG_AKV_PLUGIN_MANAGED_IDENTITY_NOT_FOUND(objectId.c_str());

    }
    else
    {
        status = E_FAIL;

        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: The access token request failed. HTTP status code: %d.",
            response.status_code()
        );

        EventWriteCCG_AKV_PLUGIN_HTTP_REQUEST_FOR_ACCESS_TOKEN_FAILED(objectId.c_str(),response.status_code());
    }

    return status;

}

///
/// Parses a JSON string that contains the response of the Azure Key vault
/// request and retrieves the plain text credentials.
///
/// \param ResponseBody        The JSON string with the Key Vault response.
/// \param SecretValue         The output parameter, that will contain the
///     recovered plain text credentials.
///
/// \return True if the secret value attribute was found in the JSON, and
///     successfully retrieved; Otherwise, returns false.
///
bool
GetSecretValueFromResponse(
    _In_ std::wstring& ResponseBody,
    _Inout_ std::wstring& SecretValue
)
{
    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        "%!FUNC!: GetSecretValueFromResponse called."
    );

    JsonFileParser jsonParser(ResponseBody);

    bool valueFound = false;

    try
    {
        if (jsonParser.BeginParseObject())
        {
            do
            {
                const std::wstring key(jsonParser.GetKey());

                if (_wcsicmp(key.c_str(), L"value") == 0)
                {
                    const std::wstring& value = jsonParser.ParseStringValue();
                    //
                    // The response is a wide string, but it only contains 8-bits
                    // UTF8 characters. So, it is necessary to convert it to a
                    // normal string, to use the MultiByteToWideChar 
                    //
                    std::string utf8Crdentials = AnsiWstringToString(value);
                    auto zeroCredentials = wil::SecureZeroMemory_scope_exit((void*)utf8Crdentials.c_str(), utf8Crdentials.size());

                    jsonParser.CleanValueBuffer();

                    //
                    // Convert the secret's value from UTF8 to UTF16.
                    //
                    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8Crdentials.c_str(), (int)utf8Crdentials.length(), NULL, 0);
                    SecretValue.resize(size_needed);

                    MultiByteToWideChar(CP_UTF8, 0, (LPCCH)utf8Crdentials.c_str(), (int)utf8Crdentials.length(), (LPWSTR)(SecretValue.c_str()), size_needed);
                    //_setmode(_fileno(stdout), _O_U16TEXT);
                    //wprintf(L"Secret value: %s (%d)\n", SecretValue.c_str(), (int)SecretValue.size());

                    //
                    // The value tag was found.
                    //
                    valueFound = true;
                }
                else
                {
                    jsonParser.SkipValue();
                }

            } while (jsonParser.ParseNextObjectElement());
        }
    }
    catch (...)
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: Error parsing the Json."
        );
        EventWriteCCG_AKV_PLUGIN_GET_SECRET_VALUE_FROM_RESPONSE_JSON_ERROR();

        return false;
    }

    if (!valueFound)
    {
        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: Tag 'value' wasn't found."
        );
        EventWriteCCG_AKV_PLUGIN_MISSING_SECRET();
    }

    return valueFound;
}

///
/// Returns a task with the request to Azure Key Vault, to obtain the plaintext
/// credentials.
///
/// \param AccessToken         The access token, retrieved by calling to MSI.
/// \param domainName          An output parameter, that contains the domainName
/// \param username            An output parameter, that contains the username
/// \param password            An output parameter, that contains the password
///
/// \return A task that requests the credentials, and will return a status
///     code with the result.
///
HRESULT
RequestSecret(
    _In_ const std::wstring& AccessToken,
    _In_ std::wstring secretURI, 
    _Outptr_ LPWSTR* domainName,
    _Outptr_ LPWSTR* username,
    _Outptr_ LPWSTR* password

)
{
    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        "%!FUNC!: RequestSecret called."
    );
    EventWriteCCG_AKV_PLUGIN_REQUESTING_SECRET_FROM_KEYVAULT();

    web::http::client::http_client client(secretURI);
   
    web::http::uri_builder builder(U("/"));
    builder.append_query(U("api-version"), U("7.1"));

    //
    // Instantiate our own response buffer, to be able to clean its memory
    // after using it.
    //
    auto secretRequestBuffer = std::make_shared< Concurrency::streams::container_buffer<std::vector<uint8_t>> >();

    web::http::http_request request(web::http::methods::GET);

    request.set_request_uri(builder.to_uri());
    request.set_response_stream(secretRequestBuffer->create_ostream());
    request.headers().add(U("Authorization"), AccessToken);

    //std::wcout << L"Requesting secret to:" << (PWCHAR)client.base_uri().to_string().c_str() << (PWCHAR)request.absolute_uri().to_string().c_str() << L"\n";

    //
    // TODO: this line prevents cpprestsdk to crash. It looks like a bug.
    // Remove when fixed
    //
    utility::string_t abs_uri = request.absolute_uri().to_string();

    web::http::http_response response = client.request(request).get();
    HRESULT status = S_OK;

    if (response.status_code() == 200)
    {
        
        response.content_ready().get();

        std::wstring responseBody;
        auto zeroResponseBody = SecureZeroMemoryScopeExit(responseBody);
        auto zeroBuffer = wil::SecureZeroMemory_scope_exit(
            &(secretRequestBuffer->collection())[0],
            secretRequestBuffer->collection().size() * sizeof((secretRequestBuffer->collection())[0]));

        responseBody.assign(
            &(secretRequestBuffer->collection())[0],
            &(secretRequestBuffer->collection())[0] + secretRequestBuffer->collection().size()
        );

        std::wstring secretValue;
        auto zeroSecretValue = SecureZeroMemoryScopeExit(secretValue);

        //
        // Extract the secret's value (that contains the credentials)
        // from the JSON response.
        //
        bool success = GetSecretValueFromResponse(responseBody, secretValue);
        

        if (success)
        {

            size_t colonIndex = secretValue.find(L":");
            size_t backslashIndex = secretValue.find(L"\\");

            //
            // Check if the secret's value follows the "domain\username:password" format.
            //
            if (colonIndex != wstring::npos &&
                backslashIndex != wstring::npos &&
                colonIndex > backslashIndex)
            {
                std::wstring domain = secretValue.substr(0, backslashIndex);
                std::wstring usernameFound = secretValue.substr(backslashIndex + 1, colonIndex - backslashIndex - 1);
                auto zeroUsername = SecureZeroMemoryScopeExit(usernameFound);

                std::wstring passwordFound = secretValue.substr(colonIndex + 1);
                auto zeroPassword = SecureZeroMemoryScopeExit(passwordFound);


                auto userCo = wil::make_cotaskmem_string_nothrow(usernameFound.c_str());
                auto passwordCo = wil::make_cotaskmem_string_nothrow(passwordFound.c_str());
                auto domainCo = wil::make_cotaskmem_string_nothrow(domain.c_str());
                if (userCo == nullptr || passwordCo == nullptr || domainCo == nullptr)
                {
                    return STG_E_INSUFFICIENTMEMORY;
                }

                *domainName = domainCo.release();
                *username = userCo.release();
                *password = passwordCo.release();

            }
            else
            {
                status = E_INVALIDARG;

                TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_DLL,
                    "%!FUNC!: The secret doesn't have the 'domain\\username:password' format.."
                );
                EventWriteCCG_AKV_PLUGIN_INVALID_SECRET_FORMAT();
            }
        }
        else
        {
            status = E_FAIL;

            TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_DLL,
                "%!FUNC!: Retrieving the secret from the response failed. "
            );
        }
    }
    else if (response.status_code() == 403)
    {
        status = E_ACCESSDENIED;

        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: Key vault access denied."
        );

        EventWriteCCG_AKV_PLUGIN_KEY_VAULT_ACCESS_DENIED();
    }
    else
    {
        status = E_FAIL;

        TraceEvents(
            TRACE_LEVEL_ERROR,
            TRACE_DLL,
            "%!FUNC!: The key vault request failed. HTTP status code: %d.",
            response.status_code()
        );

        EventWriteCCG_AKV_PLUGIN_REQUEST_TO_KEY_VAULT_FAILED(response.status_code());
    }

    return status;
    

}

///  Retrieves credentials from a Key Vault.
///
/// \param domainName      The out parameter to hold domain.
/// \param username        The out parameter to hold username.
/// \param passwrod        The out parameter to hold passwrod. 
///
/// \return STATUS_SUCCESS if it's completed successfully. Other HRESULT value
///     with the error, if it fails.
///
HRESULT RetrieveVaultCredentials(
    _In_ std::wstring objectId, 
    _In_ std::wstring secretURI,
    _Outptr_ LPWSTR* domainName,
    _Outptr_ LPWSTR* username,
    _Outptr_ LPWSTR* password)
{
    HRESULT status = S_OK;

    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        "%!FUNC!: RetrieveVaultCredentials called. Secret URI is %S.",
        secretURI.c_str()
    );

    bool isSecretUriEmpty = secretURI.empty();

    if (isSecretUriEmpty)
    {
        status = E_NOT_VALID_STATE;
    }
    
   
    if (status == S_OK)
    {
        std::wstring accessToken;
        auto zeroPassword = SecureZeroMemoryScopeExit(accessToken);

        size_t dotindex = secretURI.find(L".");
        size_t slashindex ;
        if (dotindex != wstring::npos){
            slashindex = secretURI.substr(dotindex + 1).find(L"/");
            if (slashindex == wstring::npos){
                status = E_INVALIDARG;
                EventWriteCCG_AKV_PLUGIN_INVALID_RESOURCE_IN_SECRETURI();
            }
        }
        else{
            status =  E_INVALIDARG;
            EventWriteCCG_AKV_PLUGIN_INVALID_RESOURCE_IN_SECRETURI();
        }
       
        if (status == S_OK)
        {

            try
            {
                LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
                LARGE_INTEGER Frequency;

                QueryPerformanceFrequency(&Frequency);
                QueryPerformanceCounter(&StartingTime);
                std::wstring resource = L"https://" + secretURI.substr(dotindex + 1, slashindex);
              
                status = RequestAccessToken(objectId, resource, accessToken);

                QueryPerformanceCounter(&EndingTime);
                ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

                ElapsedMicroseconds.QuadPart *= 1000000;
                ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

                TraceEvents(
                    TRACE_LEVEL_INFORMATION,
                    TRACE_DLL,
                    "%!FUNC!: RequestAccessToken took %lld(microseconds).",
                    ElapsedMicroseconds.QuadPart
                );

            }
            catch (const std::exception e)
            {
                TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_DLL,
                    "%!FUNC!: RequestAccessToken failed. Error: %s.",
                    e.what()
                );

                EventWriteCCG_AKV_PLUGIN_ACCESS_TOKEN_REQUEST_FAILED((PCWSTR)e.what());

                status = E_FAIL;
            }

            if (status == S_OK)
            {
                try
                {
                    LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
                    LARGE_INTEGER Frequency;

                    QueryPerformanceFrequency(&Frequency);
                    QueryPerformanceCounter(&StartingTime);

                    for (int i = 0; i < 5; i++) {
                        try
                        {
                            status = RequestSecret(accessToken, secretURI, domainName, username, password);
                            break;
                        }
                        catch (web::http::http_exception  e)
                        {
                            TraceEvents(
                                TRACE_LEVEL_ERROR,
                                TRACE_DLL,
                                "%!FUNC!: error caught after requestSecret. Error: %s.",
                                e.what());


                            if (e.error_code().value() != 12007) {
                                break;
                            }
                        }
                        TraceEvents(
                            TRACE_LEVEL_INFORMATION,
                            TRACE_DLL,
                            "%!FUNC!: Retrying RequestSecret after 5 seconds."
                        );


                        if (i == 4) {
                            break;
                        }
                        Sleep(5000);
                    }


                    QueryPerformanceCounter(&EndingTime);
                    ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

                    ElapsedMicroseconds.QuadPart *= 1000000;
                    ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

                    TraceEvents(
                        TRACE_LEVEL_INFORMATION,
                        TRACE_DLL,
                        "%!FUNC!: RequestSecret took %lld(microseconds).",
                        ElapsedMicroseconds.QuadPart);

                }
                catch (const std::exception e)
                {
                    TraceEvents(
                        TRACE_LEVEL_ERROR,
                        TRACE_DLL,
                        "%!FUNC!: RequestSecret failed. Error: %s.",
                        e.what());
                    EventWriteCCG_AKV_PLUGIN_SECRET_REQUEST_FAILED((PCWSTR)e.what());

                    status = E_FAIL;
                }
            }
        }
    }

    TraceEvents(
        TRACE_LEVEL_INFORMATION,
        TRACE_DLL,
        L"%!FUNC! completed. Status: %d.",
        status);
    
    EventWriteCCG_AKV_PLUGIN_FINISHED_RETRIEVING_CREDENTIALS(status);

    return status;
}

//[uuid("ccc2a336-d7f3-4818-a213-272b7924213e")]
class CCGPlugin : public RuntimeClass <RuntimeClassFlags<RuntimeClassType::ClassicCom>, ICcgDomainAuthCredentials>
{
public:
    CCGPlugin() {}

    ~CCGPlugin() {}

    IFACEMETHODIMP GetPasswordCredentials(
        _In_ LPCWSTR pluginInput,
        _Outptr_ LPWSTR* domainName,
        _Outptr_ LPWSTR* username,
        _Outptr_ LPWSTR* password)
    {
        TraceEvents(
            TRACE_LEVEL_INFORMATION,
            TRACE_DLL,
            "%!FUNC!: Initializing plugin with plugin input %S",
            pluginInput
        );

        EventWriteCCG_AKV_PLUGIN_RECEIVED_REQUEST();


        std::error_code err;
        HRESULT status = S_OK;
        
        if(pluginInput == nullptr)
        {

            EventWriteCCG_AKV_PLUGIN_MISSING_PLUGIN_INPUT();

            return E_INVALIDARG;
        }

        LPCWSTR ptr = pluginInput;
        LPCWSTR equalIndex, semicolonIndex;
        static utility::string_t secretURI;
        static utility::string_t objectId;

        while (*ptr)
        {
            equalIndex = wcschr(ptr, L'=');
            if (equalIndex == nullptr)
            {
                break;
            }

            semicolonIndex = wcschr(equalIndex, L';');

            auto keySize = equalIndex - ptr;

            //
            // Check if the actual key is equal to any predefined key. It's necessary
            // to check that the size matches before comparing it, to avoid match
            // with a prefix instead of the full name.
            //
            if (keySize == (_countof(CCGAKVPLUGIN_OBJECT_ID) - 1) &&
                _wcsnicmp(ptr, CCGAKVPLUGIN_OBJECT_ID, keySize) == 0)
            {
                
                objectId = (semicolonIndex != nullptr) ?
                    utility::string_t(equalIndex + 1, semicolonIndex) :
                    utility::string_t(equalIndex + 1);

            }
            else if (keySize == (_countof(CCGAKVPLUGIN_SECRET_URI) - 1) &&
                _wcsnicmp(ptr, CCGAKVPLUGIN_SECRET_URI, keySize) == 0)
            {
                secretURI = (semicolonIndex != nullptr) ?
                    utility::string_t(equalIndex + 1, semicolonIndex) :
                    utility::string_t(equalIndex + 1);
            }

            if (semicolonIndex == nullptr)
            {
                break;
            }
            else
            {
                ptr = semicolonIndex + 1;
            }
        }

        bool isObjectIdEmpty = objectId.empty();
        bool isSecretUriEmpty = secretURI.empty();
      
        if (isObjectIdEmpty)
        {
            TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_DLL,
                "%!FUNC!: ObjectId is empty");
            EventWriteCCG_AKV_PLUGIN_EMPTY_OBJECTID();

            status = E_INVALIDARG;
        }
        else if (isSecretUriEmpty)
        {
            TraceEvents(
                TRACE_LEVEL_ERROR,
                TRACE_DLL,
                "%!FUNC!: Secret URI is empty");

            EventWriteCCG_AKV_PLUGIN_EMPTY_SECRETURI();

            status = E_INVALIDARG;
        }
        else
        {
            try
            {
                //
                // Check if the given URI is valid.
                //
                web::http::client::http_client client(secretURI);
            }
            catch (...)
            {
                TraceEvents(
                    TRACE_LEVEL_ERROR,
                    TRACE_DLL,
                    "%!FUNC!: The URI '%S' isn't valid",
                    secretURI.c_str());

                EventWriteCCG_AKV_PLUGIN_INVALID_SECRETURI();

                status = E_INVALIDARG;
            }

        }

        TraceEvents(
            TRACE_LEVEL_INFORMATION,
            TRACE_DLL,
            "%!FUNC!: Object ID is  %S . Secret URI is %S",
            objectId.c_str(),
            secretURI.c_str()
        );

        if (status != S_OK) {
            return status;
        }      
        status = RetrieveVaultCredentials(objectId, secretURI, domainName, username, password);
        
        return status;
        
    }
};
CoCreatableClass(CCGPlugin);