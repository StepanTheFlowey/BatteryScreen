#pragma once

#include <string>
#include <SetupAPI.h>
#include <batclass.h>
#include <devguid.h>
#include "wmi.hpp"

#pragma comment(lib, "Setupapi.lib")

class Battery {
  std::wstring name;
public:
  BATTERY_STATUS status {};
  BATTERY_INFORMATION info {};
  SYSTEM_POWER_STATUS powerStatus {};
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
    return static_cast<float>(status.Capacity) / (info.FullChargedCapacity);
  }

  void update() {
    if(error) return;

    error = !GetSystemPowerStatus(&powerStatus);

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
    if(hBattery == INVALID_HANDLE_VALUE) {
      error = true;
      return;
    }

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

    if(error) return;

    if(!DeviceIoControl(hBattery,
       IOCTL_BATTERY_QUERY_INFORMATION,
       &bqi,
       sizeof(bqi),
       &info,
       sizeof(info),
       &dwBytesReturned,
       NULL))
      error = true;

    if(error) return;

    bws.BatteryTag = bqi.BatteryTag;
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

#ifdef DEBUG
    _wsystem(L"cls");
    std::wcout << L"Name:\t" << name << std::endl;
    std::wcout << L"Charge:\t" << getCharge() * 100 << L"%" << std::endl;
    std::wcout << L"BATTERY_STATUS:" << std::endl;
    std::wcout << L"  PowerState:" << std::endl;
    if(status.PowerState & BATTERY_POWER_ON_LINE)
      std::wcout << L"    BATTERY_POWER_ON_LINE" << std::endl;
    if(status.PowerState & BATTERY_DISCHARGING)
      std::wcout << L"    BATTERY_DISCHARGING" << std::endl;
    if(status.PowerState & BATTERY_CHARGING)
      std::wcout << L"    BATTERY_CHARGING" << std::endl;
    if(status.PowerState & BATTERY_CRITICAL)
      std::wcout << L"    BATTERY_CRITICAL" << std::endl;
    std::wcout << L"  Capacity:\t" << status.Capacity << std::endl;
    std::wcout << L"  Voltage:\t" << status.Voltage << std::endl;
    std::wcout << L"  Rate:\t\t" << status.Rate << std::endl;
    std::wcout << L"BATTERY_INFORMATION:" << std::endl;
    std::wcout << L"  Capabilities: " << std::endl;
    if(info.Capabilities & BATTERY_SYSTEM_BATTERY)
      std::wcout << L"    BATTERY_SYSTEM_BATTERY" << std::endl;
    if(info.Capabilities & BATTERY_CAPACITY_RELATIVE)
      std::wcout << L"    BATTERY_CAPACITY_RELATIVE" << std::endl;
    if(info.Capabilities & BATTERY_IS_SHORT_TERM)
      std::wcout << L"    BATTERY_IS_SHORT_TERM" << std::endl;
    if(info.Capabilities & BATTERY_SEALED)
      std::wcout << L"    BATTERY_SEALED" << std::endl;
    if(info.Capabilities & BATTERY_SET_CHARGE_SUPPORTED)
      std::wcout << L"    BATTERY_SET_CHARGE_SUPPORTED" << std::endl;
    if(info.Capabilities & BATTERY_SET_DISCHARGE_SUPPORTED)
      std::wcout << L"    BATTERY_SET_DISCHARGE_SUPPORTED" << std::endl;
    if(info.Capabilities & BATTERY_SET_CHARGINGSOURCE_SUPPORTED)
      std::wcout << L"    BATTERY_SET_CHARGINGSOURCE_SUPPORTED" << std::endl;
    if(info.Capabilities & BATTERY_SET_CHARGER_ID_SUPPORTED)
      std::wcout << L"    BATTERY_SET_CHARGER_ID_SUPPORTED" << std::endl;
    std::wcout << L"  Technology:\t\t" << info.Technology << std::endl;
    wchar_t buff[5] {};
    for(uint8_t i = 0; i < 4; i++)buff[i] = info.Chemistry[i];
    std::wcout << L"  Chemistry:\t\t" << buff << std::endl;
    std::wcout << L"  DesignedCapacity:\t" << info.DesignedCapacity << std::endl;
    std::wcout << L"  FullChargedCapacity:\t" << info.FullChargedCapacity << std::endl;
    std::wcout << L"  DefaultAlert1:\t" << info.DefaultAlert1 << std::endl;
    std::wcout << L"  DefaultAlert2:\t" << info.DefaultAlert2 << std::endl;
    std::wcout << L"  CriticalBias:\t\t" << info.CriticalBias << std::endl;
    std::wcout << L"  CycleCount:\t\t" << info.CycleCount << std::endl;
    std::wcout << L"SYSTEM_POWER_STATUS:" << std::endl;
    std::wcout << L"  ACLineStatus: " << std::endl;
    if(powerStatus.ACLineStatus & AC_LINE_OFFLINE)
      std::wcout << L"    AC_LINE_OFFLINE" << std::endl;
    if(powerStatus.ACLineStatus & AC_LINE_ONLINE)
      std::wcout << L"    AC_LINE_ONLINE" << std::endl;
    if(powerStatus.ACLineStatus & AC_LINE_BACKUP_POWER)
      std::wcout << L"    AC_LINE_BACKUP_POWER" << std::endl;
    if(powerStatus.ACLineStatus == AC_LINE_UNKNOWN)
      std::wcout << L"    AC_LINE_UNKNOWN" << std::endl;
    std::wcout << L"  BatteryFlag: " << std::endl;
    if(powerStatus.ACLineStatus & 1)
      std::wcout << L"    High" << std::endl;
    if(powerStatus.ACLineStatus & 2)
      std::wcout << L"    Low" << std::endl;
    if(powerStatus.ACLineStatus & 4)
      std::wcout << L"    Critical" << std::endl;
    if(powerStatus.ACLineStatus & 8)
      std::wcout << L"    Charging" << std::endl;
    if(powerStatus.ACLineStatus & 128)
      std::wcout << L"    System battery" << std::endl;
    if(powerStatus.ACLineStatus == 255)
      std::wcout << L"    Unknown" << std::endl;
    std::wcout << L"  BatteryLifePercent:\t" << powerStatus.BatteryLifePercent << std::endl;
    std::wcout << L"  SystemStatusFlag:\t" << powerStatus.SystemStatusFlag << std::endl;
    std::wcout << L"  BatteryLifeTime:\t" << powerStatus.BatteryLifeTime << std::endl;
    std::wcout << L"  BatteryFullLifeTime:\t" << powerStatus.BatteryFullLifeTime << std::endl;
#endif // DEBUG
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