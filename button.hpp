#pragma once

#include <SFML/Graphics.hpp>

class Button : public sf::Drawable {
  sf::Sprite sprite_;
  bool selected_ = false;
  bool pressed_ = false;
  sf::IntRect normalRect_;
  sf::IntRect selectedRect_;
  sf::IntRect pressedRect_;
  sf::IntRect rect_;
public:

  Button() {
  
  }

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

  inline void reset() {
    pressed_ = false;
    selected_ = false;
  }

  void setPosition(sf::IntRect rect) {
    sprite_.setPosition(rect.left, rect.top);
    rect_ = rect;
  }

  bool update(sf::Event& event) {
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
    return pressed_;
  }

  virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(sprite_);
  }
};