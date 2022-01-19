#define _WIN32_DCOM
#define NOMINMAX
#include <iostream>
#include <Windows.h>
#include <SFML/Graphics.hpp>

#include "resource.h"
#include "battery.hpp"
#include "button.hpp"

LONG getTaskBarHeight() {
  HWND taskBar = FindWindowW(L"Shell_traywnd", NULL);
  if(!taskBar) return 0;

  RECT rect;
  GetWindowRect(taskBar, &rect);
  return rect.bottom - rect.top;
}

#define WINDOW_SIZE_X 270
#define WINDOW_SIZE_Y 130
#define WINDOW_SIZE_X_ADV 300
#define WINDOW_SIZE_Y_ADV 300

#define BATTARY_POS_X 10
#define BATTARY_POS_Y 40
#define BATTARY_POS_X_ADV 70
#define BATTARY_POS_Y_ADV 70

#ifdef DEBUG

namespace _main {
  bool needRedraw = true;
  bool isShows = true;
  bool isAdvenced = false;

  Battery battery;

  NOTIFYICONDATAW notifyIconData {};
  HICON hIcon[16] {};
  uint8_t lastNotifyIcon = 0;

  sf::Time time;
  sf::Clock clock;
  sf::Event event;
  sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();

  sf::RenderWindow window;
  sf::Font font;
  sf::Texture texture;

  Button closeButton(texture,
                     sf::IntRect(0, 0, 30, 30), sf::IntRect(30, 0, 30, 30),
                     sf::IntRect(60, 0, 30, 30), sf::IntRect(WINDOW_SIZE_X - 30, 0, 30, 30));
  Button hideButton(texture,
                    sf::IntRect(0, 30, 30, 30), sf::IntRect(30, 30, 30, 30),
                    sf::IntRect(60, 30, 30, 30), sf::IntRect(WINDOW_SIZE_X - 60, 0, 30, 30));
  Button moreButton(texture,
                    sf::IntRect(90, 0, 30, 30), sf::IntRect(120, 0, 30, 30),
                    sf::IntRect(150, 0, 30, 30), sf::IntRect(0, 0, 30, 30));

  sf::Sprite percentSprites[4] {};
  sf::Sprite batterySprite;
  sf::Sprite chargingSprite;
  sf::Vertex chargeVertex[4] {};
  sf::Vector2<uint16_t> batteryPos;
  sf::Vector2<uint16_t> windowSize;

  void init() {
    HRSRC hResource = NULL;
    HGLOBAL hMemory = NULL;
    const HINSTANCE hInstance = GetModuleHandleW(NULL);
    hResource = FindResourceW(hInstance, MAKEINTRESOURCEW(IDB_PNG1), L"PNG");
    if(!hResource) {
      exit(EXIT_FAILURE);
    }
    hMemory = LoadResource(hInstance, hResource);
    if(!hMemory) {
      exit(EXIT_FAILURE);
    }
    texture.loadFromMemory(LockResource(hMemory), SizeofResource(hInstance, hResource));

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

    font.loadFromFile("C:\\Windows\\Fonts\\arial.ttf");

    chargeVertex[0].position = sf::Vector2f(BATTARY_POS_X, BATTARY_POS_Y);
    chargeVertex[1].position = sf::Vector2f(BATTARY_POS_X, BATTARY_POS_Y + 80);
  }

  void position() {
    batteryPos.x = isAdvenced ? BATTARY_POS_X_ADV : BATTARY_POS_X;
    batteryPos.y = isAdvenced ? BATTARY_POS_Y_ADV : BATTARY_POS_Y;

    for(uint8_t i = 0; i < 4; i++) {
      percentSprites[i].setTexture(texture);
      percentSprites[i].setTextureRect(sf::IntRect(180, 0, 20, 20));
      percentSprites[i].setPosition(batteryPos.x + 100 + i * 20, batteryPos.y + 30);
    }

    windowSize.x = isAdvenced ? WINDOW_SIZE_X_ADV : WINDOW_SIZE_X;
    windowSize.y = isAdvenced ? WINDOW_SIZE_Y_ADV : WINDOW_SIZE_Y;

    window.create(sf::VideoMode(windowSize.x, windowSize.y), "BattaryScreen", sf::Style::None);
    window.setPosition(sf::Vector2i(desktopMode.width - windowSize.x, desktopMode.height - windowSize.y - getTaskBarHeight()));
    window.setVerticalSyncEnabled(true);
  }

  void update() {
    time += clock.restart();
    if(time > sf::milliseconds(250)) {
      time = sf::microseconds(0);
      needRedraw = true;

      battery.update();
      if(battery.error) {
        chargeVertex[2].position = sf::Vector2f(batteryX + 250, batteryY + 80);
        chargeVertex[3].position = sf::Vector2f(batteryX + 250, batteryY);
        chargeVertex[0].color = sf::Color(255, 128, 128);
        chargeVertex[1].color = sf::Color(255, 0, 0);
        chargeVertex[2].color = sf::Color(255, 0, 0);
        chargeVertex[3].color = sf::Color(255, 128, 128);
      }
      else {
        float x = batteryX + battery.getCharge() * 250;
        chargeVertex[2].position = sf::Vector2f(x, batteryY + 80);
        chargeVertex[3].position = sf::Vector2f(x, batteryY);
        chargeVertex[0].color = sf::Color(128, 255, 128);
        chargeVertex[1].color = sf::Color(0, 255, 0);
        chargeVertex[2].color = sf::Color(0, 255, 0);
        chargeVertex[3].color = sf::Color(128, 255, 128);
      }

      uint8_t icon = battery.error ? 15 : battery.getCharge() * 16;
      if(icon > 14) {
        icon = 15;
      }
      else {
        icon = 14 - icon;
      }
      if(lastNotifyIcon != icon) {
        notifyIconData.hIcon = hIcon[icon];
        Shell_NotifyIconW(NIM_MODIFY, &notifyIconData);
        lastNotifyIcon = icon;
      }
    }
  }

  void draw() {
    if(needRedraw && isShows) {
      needRedraw = false;

      window.clear(sf::Color::White);

      window.draw(closeButton);
      window.draw(hideButton);
      window.draw(moreButton);

      window.draw(chargeVertex, 4, sf::Quads);
      window.draw(batterySprite);
      window.draw(percentSprites[0]);
      window.draw(percentSprites[1]);
      window.draw(percentSprites[2]);
      window.draw(percentSprites[3]);
      window.draw(chargingSprite);

      window.display();
    }
    else {
      sf::sleep(isShows ? sf::milliseconds(10) : sf::seconds(1));
    }
  }
}
int main() {
  CONSOLE_CURSOR_INFO curs {};
  curs.dwSize = sizeof(curs);
  curs.bVisible = false;
  SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curs);
  SetConsoleTitleW(L"BatteryScreen debug");
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
#endif // DEBUG
  using namespace _main;

  init();

  const GUID guid {0xF105E7BB,0xD1E0,0xD1ED,{0x70,0x76,0x79,0x87,0x69,0x89,0x33,0x33}};
  notifyIconData.cbSize = sizeof(notifyIconData);
  notifyIconData.hWnd = window.getSystemHandle();
  notifyIconData.uID = 0;
  notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  notifyIconData.uCallbackMessage = WM_MOUSEMOVE;
  notifyIconData.hIcon = hIcon[15];
  wcscpy(notifyIconData.szTip, L"Double-click to open BattaryScreen");
  notifyIconData.guidItem = guid;
  Shell_NotifyIconW(NIM_ADD, &notifyIconData);
  Shell_NotifyIconW(NIM_SETVERSION, &notifyIconData);

  bool redraw = true;
  bool advenced = false;
  bool shows = true;
  uint8_t prevIcon = 15;
  while(window.isOpen()) {
    while(window.pollEvent(event)) {
      redraw = true;
      switch(event.type) {
        case sf::Event::Closed:
          window.close();
          break;
        case sf::Event::MouseMoved:
          if(event.mouseMove.x == 515 && event.mouseMove.y == 0) {
            window.setVisible(true);
            window.requestFocus();
          }
          shows = true;
          break;
      }
      if(shows) {
        if(closeButton.update(event)) {
          closeButton.reset();
          window.close();
        }

        if(hideButton.update(event)) {
          hideButton.reset();
          window.setVisible(false);
          shows = false;
        }

        if(moreButton.update(event)) {
          moreButton.reset();
          advenced = !advenced;
          if(advenced) {
            closeButton.setPosition(sf::IntRect(WINDOW_SIZE_X_ADV - 30, 0, 30, 30));
            hideButton.setPosition(sf::IntRect(WINDOW_SIZE_X_ADV - 60, 0, 30, 30));
            window.setSize(sf::Vector2u(WINDOW_SIZE_X_ADV, WINDOW_SIZE_Y_ADV));
            window.setView(sf::View(sf::FloatRect(0, 0, WINDOW_SIZE_X_ADV, WINDOW_SIZE_Y_ADV)));
            window.setPosition(sf::Vector2i(desktopMode.width - WINDOW_SIZE_X_ADV, desktopMode.height - WINDOW_SIZE_Y_ADV - getTaskBarHeight()));
          }
          else {
            closeButton.setPosition(sf::IntRect(WINDOW_SIZE_X - 30, 0, 30, 30));
            hideButton.setPosition(sf::IntRect(WINDOW_SIZE_X - 60, 0, 30, 30));
            window.setSize(sf::Vector2u(WINDOW_SIZE_X, WINDOW_SIZE_Y));
            window.setView(sf::View(sf::FloatRect(0, 0, WINDOW_SIZE_X, WINDOW_SIZE_Y)));
            window.setPosition(sf::Vector2i(desktopMode.width - WINDOW_SIZE_X, desktopMode.height - WINDOW_SIZE_Y - getTaskBarHeight()));
          }
        }
      }
    }


    if(redraw && shows) {
      redraw = false;
      window.clear(sf::Color::White);
      window.draw(closeButton);
      window.draw(hideButton);
      window.draw(moreButton);
      window.draw(chargeVertex, 4, sf::Quads);
      window.draw(batterySprite);
      for(auto& i : chargeSprite) window.draw(i);
      window.draw(chargingSprite);
      window.display();
    }
    else {
      sf::sleep(shows ? sf::milliseconds(10) : sf::seconds(1));
    }
  }

  Shell_NotifyIconW(NIM_DELETE, &notifyIconData);
  return 0;
}