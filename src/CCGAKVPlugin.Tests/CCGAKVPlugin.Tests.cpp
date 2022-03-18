

#include "pch.h"
#include "CppUnitTest.h"
#include "../CCGAKVPlugin/CCGPlugin.h"
#include "../CCGAKVPlugin/JsonFileParser.h"
#include "../CCGAKVPlugin/JsonFileParser.cpp"

#include "atlbase.h"
#include <fstream>
#include <sstream>
#include <codecvt>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CCGAKVPluginTests
{

    //std::wstring CCGAKVPluginTests::objectIDvalue;
    TEST_CLASS(CCGAKVPluginTests)
    {
    public:

        static std::wstring readFile(const char* filename)
        {
            std::wifstream wif(filename);
            wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
            std::wstringstream wss;
            wss << wif.rdbuf();
            return wss.str();
        }
        static std::wstring objectID;
        static std::wstring validSecretUri;
        static std::wstring objectIDWithoutPermissions;
        static std::wstring illFormedValueSecretUri;
        static std::wstring invalidDomainValueSecretUri;
        static std::wstring noDomainValueSecretUri;
       

        TEST_CLASS_INITIALIZE(readParameters)
        {
           // std::ifstream myfile("CCGAKVPluginTestParameters.json");
            std::wstring mystring = readFile("CCGAKVPluginRuntimeParameters.json");
       
            JsonFileParser jsonParser(mystring);
            try
            {
                if (jsonParser.BeginParseObject())
                {
                    do
                    {
                        const std::wstring key(jsonParser.GetKey());

                        if (_wcsicmp(key.c_str(), L"ObjectID") == 0)
                        {
                            objectID = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else if (_wcsicmp(key.c_str(), L"ValidSecretUri") == 0)
                        {
                            validSecretUri = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else if (_wcsicmp(key.c_str(), L"ObjectIDWithoutPermissions") == 0)
                        {
                            objectIDWithoutPermissions = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else if (_wcsicmp(key.c_str(), L"IllFormedValueSecretUri") == 0)
                        {
                            illFormedValueSecretUri = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else if (_wcsicmp(key.c_str(), L"InvalidDomainValueSecretUri") == 0)
                        {
                            invalidDomainValueSecretUri = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else if (_wcsicmp(key.c_str(), L"NoDomainValueSecretUri") == 0)
                        {
                            noDomainValueSecretUri = jsonParser.ParseStringValue();
                            jsonParser.CleanValueBuffer();
                        }
                        else {
                            jsonParser.SkipValue();
                        }

                    } while (jsonParser.ParseNextObjectElement());
                }
            }
            catch (...)
            {
                Assert::Fail(L"Failed to parse json with runtime parameters.");
            }
           


        }
        
        
        TEST_METHOD(EmptyParameterTest)
        {
            
            std::wstring pluginInput = L"";

            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);
            
            hr = ccgPlugin->GetPasswordCredentials(NULL, &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);

        }
        
        TEST_METHOD(InvalidUriInitializeTest)
        {
            
            std::wstring secretUri = L"non-a-url";

            std::wstring pluginInput = L"ObjectId=" + objectID + L";SecretUri=" + secretUri;

            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);

        }
        
        TEST_METHOD(SuccessRetrieveTest)
        {

            std::wstring pluginInput = L"ObjectId=" + objectID + L";SecretUri=" + validSecretUri;

            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(S_OK, hr);
           
            //
            // Retrieve again to validate it didn't break.
            //
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(S_OK, hr);
        }
        
        
        TEST_METHOD(InvalidObjectIdRetrieveTest)
        {  
            //
            // A GUID that isn't a real object id .
            //
            std::wstring objectId = L"aaaaaaaa-bbbb-cccc-dddd";

            std::wstring pluginInput = L"ObjectId=" + objectId + L";SecretUri=" + validSecretUri;
          
            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);
        }
        
        TEST_METHOD(DeniedRetrieveTest)
        { 
          
            std::wstring pluginInput = L"ObjectId=" + objectIDWithoutPermissions + L";SecretUri=" + validSecretUri;
            
            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_ACCESSDENIED, hr);

           
        }
        
        
        TEST_METHOD(IllFormedValueRetrieveTest)
        {
           
            std::wstring pluginInput = L"ObjectId=" + objectID + L";SecretUri=" + illFormedValueSecretUri;
           
            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);

        }
        
        TEST_METHOD(InvalidDomainRetrieveTest)
        {
            std::wstring pluginInput = L"ObjectId=" + objectID + L";SecretUri=" + invalidDomainValueSecretUri;
           
            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);
        }

        
        TEST_METHOD(NoDomainRetrieveTest)
        {
            
            std::wstring pluginInput = L"ObjectId=" + objectID + L";SecretUri=" + noDomainValueSecretUri;
           
            LPWSTR domainName;
            LPWSTR username;
            LPWSTR password;

            CComPtr<ICcgDomainAuthCredentials> ccgPlugin;
            HRESULT hr = ccgPlugin.CoCreateInstance(__uuidof(CCGPlugin));
            hr = ccgPlugin->GetPasswordCredentials(&pluginInput[0], &domainName, &username, &password);
            Assert::AreEqual(E_INVALIDARG, hr);
        }
        
        
    };
    std::wstring CCGAKVPluginTests::objectID;
    std::wstring CCGAKVPluginTests::validSecretUri;
    std::wstring CCGAKVPluginTests::objectIDWithoutPermissions;
    std::wstring CCGAKVPluginTests::illFormedValueSecretUri;
    std::wstring CCGAKVPluginTests::invalidDomainValueSecretUri;
    std::wstring CCGAKVPluginTests::noDomainValueSecretUri;
}
