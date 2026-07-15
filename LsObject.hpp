#pragma once

#include <iostream>
#include <cmath>
#include <thread>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "HeliosDac.h"
#include "utility.hpp"
#include "globals.hpp"

class LsObject;

using EffectFunc = void(*)(LsObject*, float);
using LsValue = std::variant<bool, int, float>;

struct Vector2Ls{
    float time;
    LsValue value;
};

class Project;

class LsProperty{
private:
    LsValue configVal = 0;
    LsValue stateVal = 0;
public:
    bool isInternal = false;
    bool repsPercentage = false; //to do, float only
    int typeIndex = 0; // 0 - bool, 1 - int, 2 - float

    void resetState(){
        stateVal = configVal;
    }

    float getConfigAsFloat(){
        if(typeIndex == 0)
            return static_cast<float>(std::get<bool>(configVal));
        if(typeIndex == 1)
            return static_cast<float>(std::get<int>(configVal));
        return std::get<float>(configVal);
    }

    void setStateFromFloat(float val){
        if(typeIndex == 0)
            stateVal = static_cast<bool>(std::clamp(val, 0.f, 1.f));
        if(typeIndex == 1)
            stateVal = static_cast<int>(std::round(val));
        if(typeIndex == 2)
            stateVal = val;
    }

    template<typename T>
    bool isType(){
        return std::holds_alternative<T>(configVal);
    }

    template<typename T>
    T getState(){
        return std::get<T>(stateVal);
    }

    template<typename T>
    T getConfig(){
        return std::get<T>(configVal);
    }

    template<typename T>
    T& getStateRefr(){
        if (!std::holds_alternative<T>(configVal))
            throw std::runtime_error("getStateRefr failed - type mismatch");
        return std::get<T>(stateVal);
    }

    void setConfig(const LsValue& val){
        if(val.index() != typeIndex)
            throw std::runtime_error("LsProperty::setConfig failure - type mismatch.\n\nConfig val info:"
                                     +logLsValueInfo(configVal)+"\nState val info: "+logLsValueInfo(stateVal));
        configVal = val;
        resetState();
    }

    void setState(const LsValue& val){
        if(val.index() != typeIndex)
            throw std::runtime_error("LsProperty::setState failure - type mismatch.\n\nConfig val info:"
                                     +logLsValueInfo(configVal)+"\nState val info: "+logLsValueInfo(stateVal));
        stateVal = val;
    }

    LsProperty(){}
    LsProperty(LsValue value, bool isInternal = false){
        configVal = value;
        stateVal = value;
        this->isInternal = isInternal;
        typeIndex = value.index();
    }
};

struct LsEffectInfo{
    float defaultSizeX;
    float defaultSizeY;
    bool forceSquareShape;
    int maxColorsCount;
    std::unordered_map<std::string, LsProperty> propertySet;

    LsEffectInfo(){}
    LsEffectInfo(float defaultSizeX, float defaultSizeY, bool forceSquareShape, int maxColorsCount, std::unordered_map<std::string, LsProperty> propertySet){
        this->defaultSizeX = defaultSizeX;
        this->defaultSizeY = defaultSizeY;
        this->forceSquareShape = forceSquareShape;
        this->propertySet = propertySet;
    }
};

class LsObject{
private:
    std::unordered_map<std::string, LsProperty> effectProperties ={
        {"Pos X", LsProperty(2047.f)},
    {"Pos Y", LsProperty(2047.f)},
    {"Size X", LsProperty(4095.f)},
    {"Size Y", LsProperty(4095.f)},
    {"Rotation", LsProperty(0.f)},
    };
public:
    std::string objectName = "New Object";
    std::string effectName = "NullEffect";
    unsigned int layer = 0;

    float startTime = 0.f;
    float duration = 0.f;
    bool isInsideContainer = false;
    bool forceSquareShape = false;

    /*float posX = 2047.f;
    float posY = 2047.f;
    float sizeX = 4095.f;
    float sizeY = 4095.f;
    float rotation = 0.f;*/

    int maxColorsCount = 0;
    std::vector<sf::Color> colors = defaultColorSet;

    std::vector<HeliosPoint> points;

    std::string logInfo(){
        return "\nLsObject info:\nObject name: "+objectName+"\nEffect name: "+effectName+"Is inside container: "+std::to_string(isInsideContainer)+"\n";
    }

    bool hasProperty(const std::string& name){
        auto it = effectProperties.find(name);
        if(it == effectProperties.end())
            return false;
        return true;
    }

    LsProperty& getProperty(const std::string& name){
        auto it = effectProperties.find(name);
        if(it == effectProperties.end())
            throw std::runtime_error("getProperty failed for property '" + name + "'" + logInfo());
        return it->second;
    }

    std::unordered_map<std::string, LsProperty>& getAllProperties(){
        return effectProperties;
    }

    template<typename T>
    T& GEF(std::string name){
        if(hasProperty(name)){
            return effectProperties[name].getStateRefr<T>();
        }
        throw std::runtime_error("getPropertyStateValRefr failed for property " + name + logInfo());
    }

    EffectFunc effectFunc;
    float currentTime = 0.f; //how long the effect has been played for
    float lastRanTimestamp = 0.f;

    void resetPlayback(){
        currentTime = 0.f;
        lastRanTimestamp = 0.f;
        for(auto& propertyEntry : getAllProperties()){
            if(propertyEntry.second.isInternal)
                propertyEntry.second.resetState();
        }
    }

    void playEffect(float playbackTimestamp){
        if(!effectFunc){
            std::cout << "Missing effect function attachment in " << objectName << "\n";
            return;
        }
        bool wentBackwards = false;
        if(playbackTimestamp < lastRanTimestamp){
            resetPlayback();
            wentBackwards = true;
        }
        if(playbackTimestamp < startTime || playbackTimestamp >= startTime + duration || playbackTimestamp == lastRanTimestamp){
            lastRanTimestamp = playbackTimestamp;
            return;
        }

        float dt = wentBackwards ? playbackTimestamp - startTime : playbackTimestamp - lastRanTimestamp;
        lastRanTimestamp = playbackTimestamp;
        points.clear();
        while(dt > 0.f){
            if(dt > maxLsObjectDt){
                effectFunc(this, maxLsObjectDt);
                currentTime += maxLsObjectDt;
                dt -= maxLsObjectDt;
            }
            else{
                points.clear();
                effectFunc(this, dt);
                currentTime += dt;
                dt = 0.f;
            }
        }

        //apply rotation
        float rotationRad = getProperty("Rotation").getState<float>() * (M_PI / 180.0f);
        float cosA = std::cos(rotationRad);
        float sinA = std::sin(rotationRad);

        for (int i = 0; i < points.size(); i++) {
            float dx = points[i].x - getProperty("Pos X").getState<float>();
            float dy = points[i].y - getProperty("Pos Y").getState<float>();

            points[i].x = getProperty("Pos X").getState<float>() + (dx * cosA - dy * sinA);
            points[i].y = getProperty("Pos Y").getState<float>() + (dx * sinA + dy * cosA);
        }
    }

    void draw(std::vector<HeliosPoint>& data){
        data.insert(data.end(), points.begin(), points.end());
    }

    void initializeEffect();
    LsObject(){}
    LsObject(std::string effectName, std::string objectName, float startTime, float duration, unsigned int layer, bool isInsideContainer = false);
    //the full constructor is defined in LsObject.cpp due to access reasons
};

enum ModifierMode{
    SET,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE
};

enum ModifierType{
    LINEAR,
    EASEINSINE,
    EASEOUTSINE,
    EASEINOUTSINE,
    EASEINQUAD,
    EASEOUTQUAD,
    EASEINOUTQUAD,
    EASEINCUBIC,
    EASEOUTCUBIC,
    EASEINOUTCUBIC,
    EASEINQUART,
    EASEOUTQUART,
    EASEINOUTQUART,
    EASEINQUINT,
    EASEOUTQUINT,
    EASEINOUTQUINT,
    EASEINEXPO,
    EASEOUTEXPO,
    EASEINOUTEXPO,
    EASEINCIRC,
    EASEOUTCIRC,
    EASEINOUTCIRC,
    EASEINBACK,
    EASEOUTBACK,
    EASEINOUTBACK,
    EASEINELASTIC,
    EASEOUTELASTIC,
    EASEINOUTELASTIC,
    EASEINBOUNCE,
    EASEOUTBOUNCE,
    EASEINOUTBOUNCE
};

class LsModifier{
public:
    std::string modifierName = "New Modifier";
    unsigned int layer = 0;
    float startTime = 0.f;
    float duration = 0.f;
    float repeatCount = 0.f;
    bool transitionBack = true;
    LsValue startVal;
    LsValue endVal;
    ModifierMode mode;
    ModifierType type;

    Project* projectBelonging;
    std::unordered_set<unsigned int> linkedObjectsIDs;
    std::string linkedPropertyName;

    float getValueAt(float modTimestamp);

    void runForAllObjectsAt(float timestamp);

    void link(unsigned int ID){
        linkedObjectsIDs.emplace(ID);
    }

    LsModifier(){}
    LsModifier(std::string linkedPropertyName, float startTime, float duration, LsValue startVal, LsValue endVal, ModifierMode mode, ModifierType type, float repeatCount = 1.f, bool repeatTransition = true){
        this->linkedPropertyName = linkedPropertyName;
        this->startTime = startTime;
        this->duration = duration;
        this->startVal = startVal;
        this->endVal = endVal;
        this->mode = mode;
        this->type = type;
        this->repeatCount = repeatCount;
        this->transitionBack = repeatTransition;
    }

    LsModifier(std::string linkedPropertyName, float startTime, float duration, LsValue val, bool isRepeating = false, float transformDuration = 0.f){
        this->linkedPropertyName = linkedPropertyName;
        this->startTime = startTime;
        this->duration = duration;
        this->startVal = val;
        this->endVal = val;
        this->mode = SET;
        this->type = LINEAR;
    }
};

class LsContainer{
private:
    std::vector<LsObject*> cachedObjects;
    std::vector<LsModifier*> cachedModifiers;
public:
    std::string containerName = "New Container";

    std::unordered_set<unsigned int> heldIDs;

    float startTime = 0.f;
    float length = 0.f;
    unsigned int layer = 0;
    unsigned int numOfLayers = 1;

    LsContainer(){}
    LsContainer(const std::string& containerName, float startTime, float length, unsigned int layer, std::unordered_set<unsigned int> heldIDs = {}){
        this->containerName = containerName;
        this->startTime = startTime;
        this->length = length;
        this->layer = layer;
        this->heldIDs = heldIDs;
    }
};
