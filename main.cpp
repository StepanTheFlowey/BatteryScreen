#define _WIN32_DCOM
#define NOMINMAX
#include <iostream>
#include <comdef.h>
#include <SetupAPI.h>
#include <batclass.h>
#include <devguid.h>
#include <Wbemidl.h>
#include <Windows.h>
#include <SFML/Graphics.hpp>

#include "resource.h"

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Setupapi.lib")

class Wmi {
  HRESULT hResult = NULL;
  IWbemLocator* pLoc = nullptr;
  IWbemServices* pSvc = nullptr;
  IEnumWbemClassObject* pEnumerator = nullptr;
public:
  VARIANT* result = nullptr;

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

class Battery {
  std::wstring name;
public:
  BATTERY_STATUS status {};
  BATTERY_INFORMATION info {};
  bool error = false;

  Battery() {
    Wmi wmi;
    wmi.requestClass(L"Win32_Battery");
    wmi.requestMember(L"Name");
    name = wmi.result->bstrVal;
  }

  ~Battery() {

  }

  const std::wstring& getName() {
    return name;
  }

  float getCharge() {
    return static_cast<float>(status.Capacity - info.DefaultAlert1) / (info.FullChargedCapacity - info.DefaultAlert1);
  }

  void update() {
    if(error) return;

    const std::wstring devicePath = GetBatteryDevicePath(0);

    HANDLE hBattery
      = CreateFileW(devicePath.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);

    if(hBattery == INVALID_HANDLE_VALUE) return;
    DWORD dwBytesReturned = 0;
    DWORD dwWait = 0;
    BATTERY_QUERY_INFORMATION bqi {};
    BATTERY_WAIT_STATUS bws {0,100};
    if(!DeviceIoControl(hBattery,
       IOCTL_BATTERY_QUERY_TAG,
       &dwWait,
       sizeof(dwWait),
       &bqi.BatteryTag,
       sizeof(bqi.BatteryTag),
       &dwBytesReturned,
       NULL)
       && bqi.BatteryTag)
      error = true;
    bws.BatteryTag = bqi.BatteryTag;
    if(!error)
      if(!DeviceIoControl(hBattery,
         IOCTL_BATTERY_QUERY_INFORMATION,
         &bqi,
         sizeof(bqi),
         &info,
         sizeof(info),
         &dwBytesReturned,
         NULL))
        error = true;

    if(!error)
      if(!DeviceIoControl(hBattery,
         IOCTL_BATTERY_QUERY_STATUS,
         &bws,
         sizeof(bws),
         &status,
         sizeof(status),
         &dwBytesReturned,
         NULL))
        error = true;

    CloseHandle(hBattery);
  }
private:

  const std::wstring GetBatteryDevicePath(const int batteryIndex) {
    std::wstring result = L"";
    HDEVINFO hDeviceInfoList = SetupDiGetClassDevsW(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if(hDeviceInfoList != INVALID_HANDLE_VALUE) {
      SP_DEVICE_INTERFACE_DATA did {};
      did.cbSize = sizeof(did);

      if(SetupDiEnumDeviceInterfaces(hDeviceInfoList, 0, &GUID_DEVCLASS_BATTERY, batteryIndex, &did)) {
        DWORD dwRequiredSize = 0;

        SetupDiGetDeviceInterfaceDetailW(hDeviceInfoList, &did, 0, 0, &dwRequiredSize, 0);
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
          PSP_DEVICE_INTERFACE_DETAIL_DATA_W pdidd =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W) LocalAlloc(LPTR, dwRequiredSize);

          if(pdidd) {
            pdidd->cbSize = sizeof(PSP_DEVICE_INTERFACE_DETAIL_DATA_W);

            if(SetupDiGetDeviceInterfaceDetailW(hDeviceInfoList, &did, pdidd, dwRequiredSize, &dwRequiredSize, 0))
              result = pdidd->DevicePath;
            LocalFree(pdidd);
          }
        }
      }
      SetupDiDestroyDeviceInfoList(hDeviceInfoList);
    }

    return result;
  }
};

class Button : public sf::Drawable {
  sf::Sprite sprite_;
  bool selected_ = false;
  bool pressed_ = false;
  sf::IntRect normalRect_;
  sf::IntRect selectedRect_;
  sf::IntRect pressedRect_;
  sf::IntRect rect_;
public:

  Button(sf::Texture& texture, sf::IntRect normal, sf::IntRect selected, sf::IntRect pressed, sf::IntRect rect) {
    sprite_.setTexture(texture);
    sprite_.setTextureRect(normal);
    sprite_.setPosition(rect.left, rect.top);
    normalRect_ = normal;
    selectedRect_ = selected;
    pressedRect_ = pressed;
    rect_ = rect;
  }

  ~Button() {

  }

  bool isPressed() {
    return pressed_;
  }

  void reset() {
    pressed_ = false;
    selected_ = false;
  }

  void update(sf::Event& event) {
    switch(event.type) {
      case sf::Event::MouseMoved:
        selected_ = rect_.contains(event.mouseMove.x, event.mouseMove.y);
        if(selected_) {
          if(pressed_) break;
          sprite_.setTextureRect(selectedRect_);
        }
        else {
          pressed_ = false;
          sprite_.setTextureRect(normalRect_);
        }
        break;
      case sf::Event::MouseButtonPressed:
        pressed_ = selected_;
        if(pressed_)
          sprite_.setTextureRect(pressedRect_);
        break;
      case sf::Event::MouseButtonReleased:
        if(!pressed_) break;
        pressed_ = false;
        if(selected_)
          sprite_.setTextureRect(selectedRect_);
        else
          sprite_.setTextureRect(normalRect_);
        break;
    }
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(sprite_);
  }
};

int getTaskBarHeight() {
  RECT rect;
  HWND taskBar = FindWindowW(L"Shell_traywnd", NULL);
  if(taskBar && GetWindowRect(taskBar, &rect)) {
    return rect.bottom - rect.top;
  }
  return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
  Battery battery;

  const sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

  sf::Font font;
  font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

  sf::Texture texture;

  HRSRC hResource = NULL;
  HGLOBAL hMemory = NULL;
  hResource = FindResourceW(NULL, MAKEINTRESOURCEW(IDB_PNG1), L"PNG");
  if(!hResource) {
    return EXIT_FAILURE;
  }
  hMemory = LoadResource(NULL, hResource);
  if(!hMemory) {
    return EXIT_FAILURE;
  }
  texture.loadFromMemory(LockResource(hMemory), SizeofResource(NULL, hResource));

  HICON hIcon[16] {};
  hIcon[0] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON1), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[1] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON2), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[2] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON3), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[3] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON4), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[4] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON5), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[5] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON6), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[6] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON7), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[7] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON8), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[8] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON9), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[9] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON10), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[10] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON11), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[11] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON12), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[12] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON13), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[13] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON14), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[14] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON15), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  hIcon[15] = (HICON) LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_ICON16), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);

  Button closeButton(texture,
                     sf::IntRect(0, 0, 30, 30),
                     sf::IntRect(30, 0, 30, 30),
                     sf::IntRect(60, 0, 30, 30),
                     sf::IntRect(370, 0, 30, 30));
  Button hideButton(texture,
                    sf::IntRect(0, 30, 30, 30),
                    sf::IntRect(30, 30, 30, 30),
                    sf::IntRect(60, 30, 30, 30),
                    sf::IntRect(340, 0, 30, 30));

  sf::Sprite batterySprite(texture, sf::IntRect(0, 60, 250, 80));
  batterySprite.setPosition(75, 100);

  sf::Vertex chargeVertex[4] {};
  chargeVertex[0].position = sf::Vector2f(75, 100);
  chargeVertex[1].position = sf::Vector2f(75, 180);

  sf::Clock clock;

  sf::Time time;

  sf::Event event;

  sf::RenderWindow window(sf::VideoMode(400, 300), "BattaryScreen", sf::Style::None);
  window.setPosition(sf::Vector2i(desktopMode.width - 400, desktopMode.height - 300 - getTaskBarHeight()));
  window.setVerticalSyncEnabled(true);

  GUID guid {0xF105E7BB,0xD1E0,0xD1ED,{0x70,0x76,0x79,0x87,0x69,0x89,0x33,0x33}};
  NOTIFYICONDATAW nid {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = window.getSystemHandle();
  nid.uID = 0;
  nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid.uCallbackMessage = WM_MOUSEMOVE;
  nid.hIcon = hIcon[0];
  nid.guidItem = guid;
  wcscpy(nid.szTip, L"Double-click to open BattaryScreen™®©");
  Shell_NotifyIconW(NIM_ADD, &nid);
  Shell_NotifyIconW(NIM_SETVERSION, &nid);

  bool redraw = true;
  bool shows = true;
  while(window.isOpen()) {
    while(window.pollEvent(event)) {
      redraw = true;
      switch(event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::MouseMoved:
          if(event.mouseMove.x == 515 && event.mouseMove.y == 0)
            window.setVisible(true);
          shows = true;
          break;
      }
      if(shows) {
        closeButton.update(event);
        if(closeButton.isPressed()) {
          closeButton.reset();
          window.close();
        }

        hideButton.update(event);
        if(hideButton.isPressed()) {
          hideButton.reset();
          window.setVisible(false);
          shows = false;
        }
      }
    }

    time += clock.restart();
    if(time > sf::milliseconds(100)) {
      time = sf::microseconds(0);
      redraw = true;

      battery.update();
      if(battery.error) {
        chargeVertex[2].position = sf::Vector2f(325, 180);
        chargeVertex[3].position = sf::Vector2f(325, 100);
        chargeVertex[0].color = sf::Color(255, 128, 128);
        chargeVertex[1].color = sf::Color(255, 0, 0);
        chargeVertex[2].color = sf::Color(255, 0, 0);
        chargeVertex[3].color = sf::Color(255, 128, 128);
      }
      else {
        float x = 75 + battery.getCharge() * 250;
        chargeVertex[2].position = sf::Vector2f(x, 180);
        chargeVertex[3].position = sf::Vector2f(x, 100);
        chargeVertex[0].color = sf::Color(128, 255, 128);
        chargeVertex[1].color = sf::Color(0, 255, 0);
        chargeVertex[2].color = sf::Color(0, 255, 0);
        chargeVertex[3].color = sf::Color(128, 255, 128);
      }

      uint8_t i = battery.error ? 15 : battery.getCharge() * 16;
      if(i > 14) {
        i = 15;
      }
      else {
        i = 14 - i;
      }
      nid.hIcon = hIcon[i];
      Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    if(redraw && shows) {
      redraw = false;
      window.clear(sf::Color::White);
      window.draw(closeButton);
      window.draw(hideButton);
      window.draw(chargeVertex, 4, sf::Quads);
      window.draw(batterySprite);
      window.display();
    }
    else {
      sf::sleep(shows ? sf::milliseconds(10) : sf::seconds(1));
    }
  }

  Shell_NotifyIconW(NIM_DELETE, &nid);
  //_wsystem(L"pause");
  return 0;
}