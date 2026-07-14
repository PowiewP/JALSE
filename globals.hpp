#pragma once

#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "HeliosDac.h"

inline bool running = true;

inline const std::string appName = "JALSE";
inline const std::string projectsDir = "Projects/";
inline sf::Color backgroundColor = sf::Color(12,12,12);
inline sf::Color secondaryBackgroundColor = sf::Color(30,30,30);
inline sf::Color boxColor = sf::Color(200,0,0);
inline const std::vector<sf::Color> defaultColorSet = {
    {255, 0, 0},
    {0, 255, 0},
    {0, 0, 255},
    {255, 255, 0},
    {255, 0, 255},
    {0, 255, 255}
};
inline sf::Color defaultBackgroundColor = {200, 200, 200};
inline sf::Color defaultForegroundColor = {100, 100, 100};
inline sf::Color defaultOutlineColor = {32, 32, 32};
inline sf::Color defaultTextColor = {24, 24, 24};

inline int maxCord = 4095;
inline int minCord = 0;
inline const int maxColor = 255; //ok so we can't be doing shit like this anymore.
inline const int minColor = 0;   //Time to make a proper globals.hpp with all these things
inline int maxPPS = 15000;
inline float maxLsObjectDt = 1.f/60.f;

inline sf::Font usedFont;

class InputManager{
private:

public:
    sf::Vector2u mousePos;
    sf::Vector2u windowSize;

    bool mouseLeftJustPressed;
    bool mouseRightJustPressed;
    bool mouseLeftJustReleased;
    bool mouseRightJustReleased;
    bool mouseLeftIsHeld;
    bool mouseRightIsHeld;
    std::unordered_map<sf::Keyboard::Scancode, bool> keyJustPressed;
    std::unordered_map<sf::Keyboard::Scancode, bool> keyJustReleased;
    std::unordered_map<sf::Keyboard::Scancode, bool> keyIsHeld;

    std::string textEntered;

    float mouseWheelScrolled;

    void handleEvents(sf::RenderWindow& window){
        mouseLeftJustPressed = false;
        mouseRightJustPressed = false;
        mouseLeftJustReleased = false;
        mouseRightJustReleased = false;
        keyJustPressed.clear();
        keyJustReleased.clear();
        textEntered.clear();
        mouseWheelScrolled = 0.f;

        while (std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                running = false;
            }
            else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                windowSize = { resized->size.x, resized->size.y };
            }
            else if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
                mousePos = { (unsigned int)mouseMoved->position.x, (unsigned int)mouseMoved->position.y };
            }
            else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    mouseLeftJustPressed = true;
                    mouseLeftIsHeld = true;
                } else if (mousePressed->button == sf::Mouse::Button::Right) {
                    mouseRightJustPressed = true;
                    mouseRightIsHeld = true;
                }
            }
            else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseReleased->button == sf::Mouse::Button::Left) {
                    mouseLeftJustReleased = true;
                    mouseLeftIsHeld = false;
                } else if (mouseReleased->button == sf::Mouse::Button::Right) {
                    mouseRightJustReleased = true;
                    mouseRightIsHeld = false;
                }
            }
            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                auto code = keyPressed->scancode;
                keyJustPressed[code] = true;
                keyIsHeld[code] = true;
            }
            else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
                auto code = keyReleased->scancode;
                keyJustReleased[code] = true;
                keyIsHeld[code] = false;
            }
            else if (const auto* textEvent = event->getIf<sf::Event::TextEntered>()) {
                // Filter out non-printable characters (backspace, escape, etc.)
                if (textEvent->unicode >= 32 && textEvent->unicode < 127) {
                    textEntered += static_cast<char>(textEvent->unicode);
                }
            }
            else if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>()) {
                mouseWheelScrolled = scroll->delta;
            }
        }
    }

    InputManager(){}
};

inline static InputManager input;
