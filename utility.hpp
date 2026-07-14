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
#include "globals.hpp"

using LsValue = std::variant<bool, int, float>;

inline std::string getTodayString()
{
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

inline void drawOutlineOf(const sf::FloatRect& rect, float outlineWidth, sf::Color outlineColor, sf::RenderTarget& target){
    sf::RectangleShape line;
    line.setFillColor(outlineColor);

    line.setSize({rect.size.x, outlineWidth});

    line.setPosition(rect.position);
    target.draw(line);

    line.setPosition({rect.position.x, rect.position.y + rect.size.y - outlineWidth});
    target.draw(line);

    line.setSize({outlineWidth, rect.size.y});

    line.setPosition(rect.position);
    target.draw(line);

    line.setPosition({rect.position.x + rect.size.x - outlineWidth, rect.position.y});
    target.draw(line);
}

inline int getFontSizeOfHeight(unsigned int height) {
    if(height < 5)
        return 5;
    sf::Text temp(usedFont, "Mega gay", height+50);
    while (temp.getGlobalBounds().size.y > height) {
        temp.setCharacterSize(temp.getCharacterSize()-1);
    }
    return temp.getCharacterSize();
}

inline sf::FloatRect getReferenceBounds(const sf::Text& text) {
    sf::Text temp(text);
    temp.setString("M");
    return temp.getLocalBounds();
}

inline void centerText(sf::Text& text, const sf::Rect<unsigned int>& rect) {
    sf::FloatRect bounds = text.getLocalBounds();
    sf::FloatRect ref = getReferenceBounds(text);

    text.setOrigin({bounds.position.x + bounds.size.x / 2.f,
                     ref.position.y + ref.size.y / 2.f});

    text.setPosition({rect.position.x + rect.size.x / 2.f,
                       rect.position.y + rect.size.y / 2.f});
}

inline void centerTextVertical(sf::Text& text, const sf::Rect<unsigned int>& rect) {
    sf::FloatRect ref = getReferenceBounds(text);

    text.setOrigin({text.getOrigin().x, ref.position.y + ref.size.y / 2.f});
    text.setPosition(sf::Vector2f(text.getPosition().x, rect.position.y + rect.size.y / 2.f));
}

inline float distanceBetween(sf::Vector2f obj1, sf::Vector2f obj2) {
    return std::sqrt(((obj2.x - obj1.x) * (obj2.x - obj1.x)) + ((obj2.y - obj1.y)  * (obj2.y - obj1.y)));
}

inline float distanceBetweenNoSqrt(sf::Vector2f obj1, sf::Vector2f obj2) {
    return ((obj2.x - obj1.x) * (obj2.x - obj1.x)) + ((obj2.y - obj1.y)  * (obj2.y - obj1.y));
}

inline float angleRadiansBetween(sf::Vector2f obj1, sf::Vector2f obj2) {
    return std::atan2(obj2.y - obj1.y, obj2.x - obj1.x);
}

inline float angleDegreesBetween(sf::Vector2f obj1, sf::Vector2f obj2) {
    return angleRadiansBetween(obj1, obj2) * 180.f / 3.14159265f;
}

inline std::string logLsValueInfo(const LsValue& val){
    std::string info = "\nLsValue of type: ";
    switch(val.index()){
        case 0: info += "bool"; break;
        case 1: info += "int"; break;
        case 2: info += "float"; break;
        default: "unknown";
    }
    //info += "\nValue: to implement in log";
    return info;
}
