#pragma once

#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

class Wmi {
  HRESULT hResult = NULL;
  IWbemLocator* pLoc = nullptr;
  IWbemServices* pSvc = nullptr;
  IEnumWbemClassObject* pEnumerator = nullptr;
public:
  VARIANT* result = nullptr;
  bool error = false;

  Wmi() {
    hResult = CoInitializeEx(0, COINIT_MULTITHREADED);
    if(FAILED(hResult)) {
      std::wcout << L"Failed to initialize COM library. Error code = 0x"
        << std::hex << hResult << std::endl;
      return;
    }

    hResult = CoInitializeSecurity(
      NULL,
      -1,                          // COM authentication
      NULL,                        // Authentication services
      NULL,                        // Reserved
      RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
      RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
      NULL,                        // Authentication info
      EOAC_NONE,                   // Additional capabilities 
      NULL                         // Reserved
    );

    if(FAILED(hResult)) {
      std::wcout << L"Failed to initialize security. Error code = 0x" << std::hex << hResult << std::endl;
      CoUninitialize();
      return;
    }

    hResult = CoCreateInstance(
      CLSID_WbemLocator,
      0,
      CLSCTX_INPROC_SERVER,
      IID_IWbemLocator, (LPVOID*) &pLoc);

    if(FAILED(hResult)) {
      std::wcout << L"Failed to create IWbemLocator object."
        << L" Err code = 0x"
        << std::hex << hResult << std::endl;
      CoUninitialize();
      exit(1);
    }

    hResult = pLoc->ConnectServer(
      bstr_t(L"ROOT\\CIMV2"),  // Object path of WMI namespace
      NULL,                    // User name. NULL = current user
      NULL,                    // User password. NULL = current
      0,                       // Locale. NULL indicates current
      NULL,                    // Security flags.
      0,                       // Authority (for example, Kerberos)
      0,                       // Context object 
      &pSvc                    // pointer to IWbemServices proxy
    );

    if(FAILED(hResult)) {
      std::wcout << L"Could not connect. Error code = 0x"
        << std::hex << hResult << std::endl;
      pLoc->Release();
      CoUninitialize();
      return;
    }

    //std::wcout << "Connected to ROOT\\CIMV2 WMI namespace" << std::endl;

    hResult = CoSetProxyBlanket(
      pSvc,                        // Indicates the proxy to set
      RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
      RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
      NULL,                        // Server principal name 
      RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
      RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
      NULL,                        // client identity
      EOAC_NONE                    // proxy capabilities 
    );

    if(FAILED(hResult)) {
      std::wcout << L"Could not set proxy blanket. Error code = 0x"
        << std::hex << hResult << std::endl;
      pSvc->Release();
      pLoc->Release();
      CoUninitialize();
      return;
    }
  }

  ~Wmi() {
    if(pSvc)  pSvc->Release();
    if(pLoc) pLoc->Release();
    if(pEnumerator) pEnumerator->Release();
    CoUninitialize();
  }

  void requestClass(std::wstring name) {
    hResult = pSvc->ExecQuery(
      bstr_t("WQL"),
      bstr_t("SELECT * FROM ") + name.c_str(),
      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
      NULL,
      &pEnumerator);

    if(FAILED(hResult)) {
      std::wcout << L"Query for class failed."
        << L" Error code = 0x"
        << std::hex << hResult << std::endl;
      pSvc->Release();
      pLoc->Release();
      CoUninitialize();
      exit(1);
    }
  }

  void requestMember(std::wstring name) {
    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;

    while(pEnumerator) {
      pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

      if(0 == uReturn)
        break;

      if(result) VariantClear(result);
      result = new VARIANT;

      pclsObj->Get(name.c_str(), NULL, result, NULL, NULL);
      pclsObj->Release();
    }
  }
};