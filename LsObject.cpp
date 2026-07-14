#include <iostream>
#include <cmath>
#include <thread>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "HeliosDac.h"
#include "LsObject.hpp"
#include "utility.hpp"
#include "globals.hpp"
#include "classes.hpp"

using EffectFunc = void(*)(LsObject*, float);
using LsValue = std::variant<bool, int, float>;

void placeBlank(std::vector<HeliosPoint>& points, const sf::Vector2f& pos){
    int amountToPlace = 6;
    /*for(int i = 0; i < points.size(); i++){
        if(points[i].r != 0 || points[i].g != 0 || points[i].b != 0)
            amountToPlace++;
    }
    amountToPlace /= 10;
    amountToPlace += 1;*/

    uint16_t x = static_cast<uint16_t>(std::clamp(std::round(pos.x), static_cast<float>(minCord), static_cast<float>(maxCord)));
    uint16_t y = static_cast<uint16_t>(std::clamp(std::round(pos.y), static_cast<float>(minCord), static_cast<float>(maxCord)));

    for(int i = 0; i < amountToPlace; i++){
        points.emplace_back(HeliosPoint{x, y, 0, 0, 0, 0});
    }
}

void placePoint(std::vector<HeliosPoint>& points, const sf::Vector2f& pos, const sf::Color& color){
    uint16_t x = static_cast<uint16_t>(std::clamp(std::round(pos.x), static_cast<float>(minCord), static_cast<float>(maxCord-1)));
    uint16_t y = static_cast<uint16_t>(std::clamp(std::round(pos.y), static_cast<float>(minCord), static_cast<float>(maxCord-1)));

    uint8_t r = color.r;
    uint8_t g = color.g;
    uint8_t b = color.b;

    points.emplace_back(HeliosPoint{x, y, r, g, b, 1});
}

void playCirclingDots(LsObject* ls, float dt){
    int& numOfDots = ls->GEF<int>("Dots Count");
    bool& movingClockwise = ls->GEF<bool>("Moving Clockwise");
    float& rotationsPerSecond = ls->GEF<float>("Rotations Per Second");
    float& circleScaleX = ls->GEF<float>("Circle Scale X");
    float& circleScaleY = ls->GEF<float>("Circle Scale Y");

    float& curRotation = ls->GEF<float>("curRotation");

    float angleStep = (3.14*2) / static_cast<float>(numOfDots);
    if(movingClockwise) //M_PI stopped working. Add it back later.
        curRotation += (3.14*2) * rotationsPerSecond * dt;
    else
        curRotation -= (3.14*2) * rotationsPerSecond * dt;

    for(int i = 0; i < numOfDots; i++){
        //colors
        const sf::Color& color = ls->colors[i % ls->colors.size()];

        //position
        float angle = (angleStep * i) + curRotation;
        float floatX = ls->GEF<float>("Pos X") + std::cos(angle) * ((circleScaleX/2.f) * ls->GEF<float>("Size X"));
        float floatY = ls->GEF<float>("Pos Y") + std::sin(angle) * ((circleScaleY/2.f) * ls->GEF<float>("Size Y"));

        placeBlank(ls->points, {floatX, floatY});
        placePoint(ls->points, {floatX, floatY}, color);
        placeBlank(ls->points, {floatX, floatY});
    }
}

void playSingleDot(LsObject* ls, float dt){
    float pointRadius = ls->GEF<float>("Point Scale") * ls->GEF<float>("Size X");
    bool square = ls->GEF<bool>("Square Dot");
    float posX = ls->GEF<float>("Pos X");
    float posY = ls->GEF<float>("Pos Y");

    if(square){
        placeBlank(ls->points, {posX, posY});

        int itLeft = static_cast<int>(std::round(pointRadius)) / 2;
        int itRight = static_cast<int>(std::round(pointRadius)) / 2;
        if(static_cast<int>(std::round(pointRadius)) % 2 != 0)
            itRight++;

        for(int xP = static_cast<int>(posX) - itLeft; xP <= static_cast<int>(posX) + itRight; xP++){
            for(int yP = static_cast<int>(posY) - itLeft; yP <= static_cast<int>(posY) + itRight; yP++){
                placePoint(ls->points, {xP, yP}, ls->colors[0]);
            }
        }

        placeBlank(ls->points, {posX, posY});
    }
    else{
        placeBlank(ls->points, {posX, posY});

        int cx = static_cast<int>(std::round(posX));
        int cy = static_cast<int>(std::round(posY));
        int r  = static_cast<int>(std::round(pointRadius));

        for (int dy = -r; dy <= r; dy++) {
            int dx = static_cast<int>(std::round(std::sqrt(static_cast<float>(r * r - dy * dy))));
            for (int xP = cx - dx; xP <= cx + dx; xP++) {
                int yP = cy + dy;
                placePoint(ls->points, {xP, yP}, ls->colors[0]);
            }
        }

        placeBlank(ls->points, {posX, posY});
    }
}

void playSingleLine(LsObject* ls, float dt){
    float beginPosX = ls->GEF<float>("Pos X") - (ls->GEF<float>("Size X")/2.f);
    float beginPosY = ls->GEF<float>("Pos Y");
    float endPosX = ls->GEF<float>("Pos X") + (ls->GEF<float>("Size X")/2.f);
    float endPosY = ls->GEF<float>("Pos Y");

    placeBlank(ls->points, {beginPosX, beginPosY});
    placePoint(ls->points, {beginPosX, beginPosY}, ls->colors[0]);


    placePoint(ls->points, {endPosX, endPosY}, ls->colors[0]);
    placeBlank(ls->points, {endPosX, endPosY});
}

void playTriangle(LsObject* ls, float dt) {
    float bottomLeftX = ls->GEF<float>("Pos X") - (ls->GEF<float>("Size X")/2.f);
    float bottomLeftY = ls->GEF<float>("Pos Y") + (ls->GEF<float>("Size Y")/2.f);
    float sizeX = ls->GEF<float>("Size X");
    float sizeY = ls->GEF<float>("Size Y");

    placeBlank(ls->points, {bottomLeftX, bottomLeftY});
    placePoint(ls->points, {bottomLeftX, bottomLeftY}, ls->colors[1 % ls->colors.size()]);
    placePoint(ls->points, {bottomLeftX+sizeX, bottomLeftY}, ls->colors[1 % ls->colors.size()]);
    placePoint(ls->points, {bottomLeftX+(sizeX/2), bottomLeftY-sizeY}, ls->colors[2 % ls->colors.size()]);
    placePoint(ls->points, {bottomLeftX, bottomLeftY}, ls->colors[3 % ls->colors.size()]);
    placeBlank(ls->points, {bottomLeftX, bottomLeftY});

}

std::unordered_map<std::string, EffectFunc> effectList = {

    {"Circling Dots", playCirclingDots},
    {"Single Dot", playSingleDot},
    {"Single Line", playSingleLine},
    {"Triangle", playTriangle}

};

std::unordered_map<std::string, LsEffectInfo> effectInfo = {

    {"Circling Dots",
        LsEffectInfo(2000.f, 2000.f, false, 0,
        {
            {"Dots Count", LsProperty(8)},
            {"Rotations Per Second", LsProperty(0.4f)},
            {"Moving Clockwise", LsProperty(false)},
            {"Circle Scale X", LsProperty(1.f)},
            {"Circle Scale Y", LsProperty(1.f)},

            {"curRotation", LsProperty(0.f, true)}
        })
    },
    {"Single Dot",
        LsEffectInfo(2.f, 2.f, true, 1,
        {
            {"Point Scale", LsProperty(1.f)},
            {"Square Dot", LsProperty(false)},
        })
    },
    {"Single Line",
        LsEffectInfo(3000.f, 4000.f, true, 1,
        {

        })
    },
    {"Triangle",
        LsEffectInfo(2000.f, 2000.f, false, 3,
        {
        })
    }

};

void LsObject::initializeEffect(){
    effectFunc = effectList[effectName];
    for (const auto& [key, val] : effectInfo[effectName].propertySet) {
        effectProperties.emplace(key, val); // copies, doesn't touch the template
    }
    this->getProperty("Size X").setConfig(effectInfo[effectName].defaultSizeX);
    this->getProperty("Size Y").setConfig(effectInfo[effectName].defaultSizeX);
    this->forceSquareShape = effectInfo[effectName].forceSquareShape;
}

LsObject::LsObject(std::string effectName, std::string objectName, float startTime, float duration, unsigned int layer, bool isInsideContainer){
    this->effectName = effectName;
    this->objectName = objectName;
    this->startTime = startTime;
    this->duration = duration;
    this->layer = layer;

    initializeEffect();
}

void LsModifier::runForAllObjectsAt(float timestamp){

    if(timestamp > (duration * repeatCount * (transitionBack ? 2.f : 1.f)) + startTime || timestamp < startTime)
        return;

    for(unsigned int ID : linkedObjectsIDs){
        LsObject& obj = projectBelonging->getLsObjectByID(ID);

        float modTimestamp = timestamp - startTime;
        int repeat = 1;
        while (modTimestamp > duration) {
            modTimestamp -= duration;
            repeat++;
        }
        if (repeat % 2 == 0 && transitionBack) {
            modTimestamp = (duration - modTimestamp);
        }
        float curValF = getValueAt(modTimestamp);

        float curStateF = obj.getProperty(linkedPropertyName).getConfigAsFloat();

        switch(mode){
            case SET: curStateF = curValF; break;
            case ADD: curStateF += curValF; break;
            case SUBTRACT: curStateF -= curValF; break;
            case MULTIPLY: curStateF *= curValF; break;
            case DIVIDE: curStateF /= curValF;
        }

        obj.getProperty(linkedPropertyName).setStateFromFloat(curStateF);
    }
}

float LsModifier::getValueAt(float modTimestamp){
        float fStart = std::visit([](auto val) { return static_cast<float>(val); }, startVal);
        float fEnd   = std::visit([](auto val) { return static_cast<float>(val); }, endVal);

        // Normalised progress [0, 1]
        float x = modTimestamp / duration;
        x = std::clamp(x, 0.f, 1.f);

        // Easing constants
        constexpr float PI      = 3.14159265358979323846f;
        constexpr float c1      = 1.70158f;
        constexpr float c2      = c1 * 1.525f;
        constexpr float c3      = c1 + 1.f;
        constexpr float c4      = (2.f * PI) / 3.f;
        constexpr float c5      = (2.f * PI) / 4.5f;
        constexpr float n1      = 7.5625f;
        constexpr float d1      = 2.75f;

        // easeOutBounce as a lambda — reused by easeInBounce and easeInOutBounce
        auto easeOutBounce = [&](float t) -> float {
            if (t < 1.f / d1)
                return n1 * t * t;
            else if (t < 2.f / d1)
                return n1 * (t -= 1.5f / d1) * t + 0.75f;
            else if (t < 2.5f / d1)
                return n1 * (t -= 2.25f / d1) * t + 0.9375f;
            else
                return n1 * (t -= 2.625f / d1) * t + 0.984375f;
        };

        float t; // eased [0, 1] value
        switch(type){
            case LINEAR:
                t = x;
                break;

            // ── Sine ──────────────────────────────────────────────────────────
            case EASEINSINE:
                t = 1.f - std::cos((x * PI) / 2.f);
                break;
            case EASEOUTSINE:
                t = std::sin((x * PI) / 2.f);
                break;
            case EASEINOUTSINE:
                t = -(std::cos(PI * x) - 1.f) / 2.f;
                break;

            // ── Quad ──────────────────────────────────────────────────────────
            case EASEINQUAD:
                t = x * x;
                break;
            case EASEOUTQUAD:
                t = 1.f - (1.f - x) * (1.f - x);
                break;
            case EASEINOUTQUAD:
                t = x < 0.5f ? 2.f * x * x
                              : 1.f - std::pow(-2.f * x + 2.f, 2.f) / 2.f;
                break;

            // ── Cubic ─────────────────────────────────────────────────────────
            case EASEINCUBIC:
                t = x * x * x;
                break;
            case EASEOUTCUBIC:
                t = 1.f - std::pow(1.f - x, 3.f);
                break;
            case EASEINOUTCUBIC:
                t = x < 0.5f ? 4.f * x * x * x
                              : 1.f - std::pow(-2.f * x + 2.f, 3.f) / 2.f;
                break;

            // ── Quart ─────────────────────────────────────────────────────────
            case EASEINQUART:
                t = x * x * x * x;
                break;
            case EASEOUTQUART:
                t = 1.f - std::pow(1.f - x, 4.f);
                break;
            case EASEINOUTQUART:
                t = x < 0.5f ? 8.f * x * x * x * x
                              : 1.f - std::pow(-2.f * x + 2.f, 4.f) / 2.f;
                break;

            // ── Quint ─────────────────────────────────────────────────────────
            case EASEINQUINT:
                t = x * x * x * x * x;
                break;
            case EASEOUTQUINT:
                t = 1.f - std::pow(1.f - x, 5.f);
                break;
            case EASEINOUTQUINT:
                t = x < 0.5f ? 16.f * x * x * x * x * x
                              : 1.f - std::pow(-2.f * x + 2.f, 5.f) / 2.f;
                break;

            // ── Expo ──────────────────────────────────────────────────────────
            case EASEINEXPO:
                t = (x == 0.f) ? 0.f : std::pow(2.f, 10.f * x - 10.f);
                break;
            case EASEOUTEXPO:
                t = (x == 1.f) ? 1.f : 1.f - std::pow(2.f, -10.f * x);
                break;
            case EASEINOUTEXPO:
                t = (x == 0.f) ? 0.f
                  : (x == 1.f) ? 1.f
                  : x < 0.5f   ? std::pow(2.f,  20.f * x - 10.f) / 2.f
                                : (2.f - std::pow(2.f, -20.f * x + 10.f)) / 2.f;
                break;

            // ── Circ ──────────────────────────────────────────────────────────
            case EASEINCIRC:
                t = 1.f - std::sqrt(1.f - x * x);
                break;
            case EASEOUTCIRC:
                t = std::sqrt(1.f - std::pow(x - 1.f, 2.f));
                break;
            case EASEINOUTCIRC:
                t = x < 0.5f
                    ? (1.f - std::sqrt(1.f - std::pow(2.f * x, 2.f))) / 2.f
                    : (std::sqrt(1.f - std::pow(-2.f * x + 2.f, 2.f)) + 1.f) / 2.f;
                break;

            // ── Back ──────────────────────────────────────────────────────────
            case EASEINBACK:
                t = c3 * x * x * x - c1 * x * x;
                break;
            case EASEOUTBACK:
                t = 1.f + c3 * std::pow(x - 1.f, 3.f) + c1 * std::pow(x - 1.f, 2.f);
                break;
            case EASEINOUTBACK:
                t = x < 0.5f
                    ? (std::pow(2.f * x, 2.f) * ((c2 + 1.f) * 2.f * x - c2)) / 2.f
                    : (std::pow(2.f * x - 2.f, 2.f) * ((c2 + 1.f) * (2.f * x - 2.f) + c2) + 2.f) / 2.f;
                break;

            // ── Elastic ───────────────────────────────────────────────────────
            case EASEINELASTIC:
                t = (x == 0.f) ? 0.f
                  : (x == 1.f) ? 1.f
                  : -std::pow(2.f, 10.f * x - 10.f) * std::sin((x * 10.f - 10.75f) * c4);
                break;
            case EASEOUTELASTIC:
                t = (x == 0.f) ? 0.f
                  : (x == 1.f) ? 1.f
                  : std::pow(2.f, -10.f * x) * std::sin((x * 10.f - 0.75f) * c4) + 1.f;
                break;
            case EASEINOUTELASTIC:
                t = (x == 0.f) ? 0.f
                  : (x == 1.f) ? 1.f
                  : x < 0.5f
                    ? -(std::pow(2.f,  20.f * x - 10.f) * std::sin((20.f * x - 11.125f) * c5)) / 2.f
                    :  (std::pow(2.f, -20.f * x + 10.f) * std::sin((20.f * x - 11.125f) * c5)) / 2.f + 1.f;
                break;

            // ── Bounce ────────────────────────────────────────────────────────
            case EASEOUTBOUNCE:
                t = easeOutBounce(x);
                break;
            case EASEINBOUNCE:
                t = 1.f - easeOutBounce(1.f - x);
                break;
            case EASEINOUTBOUNCE:
                t = x < 0.5f
                    ? (1.f - easeOutBounce(1.f - 2.f * x)) / 2.f
                    : (1.f + easeOutBounce(2.f * x - 1.f)) / 2.f;
                break;

            default:
                t = x;
                break;
        }

        // Map eased [0,1] → [fStart, fEnd]
        return fStart + t * (fEnd - fStart);
}
