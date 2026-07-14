#include <iostream>
#include <fstream>
#include <cmath>
#include <complex>
#include <thread>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "HeliosDac.h"
#include "LsObject.hpp"
#include "utility.hpp"
#include "globals.hpp"

class Project{
private:
    unsigned int curObjectID = 1;
public:
    std::unordered_map<unsigned int, LsObject> objects;
    std::unordered_map<unsigned int, LsModifier> modifiers;
    std::unordered_map<unsigned int, LsContainer> containers;
    std::vector<sf::SoundBuffer> audios;

    float length = 0.f;
    std::string name = "New Project";
    std::string dateOfCreation;
    unsigned int numOfLayers = 1;

    unsigned int emplaceLsObject(LsObject obj){
        objects[curObjectID] = obj;
        curObjectID++;
        return curObjectID - 1;
    }

    unsigned int emplaceLsModifier(LsModifier mod){
        mod.projectBelonging = this;
        modifiers[curObjectID] = mod;
        curObjectID++;
        return curObjectID - 1;
    }

    LsObject& getLsObjectByID(unsigned int ID){
        auto it = objects.find(ID);
        if(it != objects.end())
            return objects[ID];
        throw std::runtime_error("getLsObjectByID failed for ID"+std::to_string(ID)+"\n");
    }

    LsObject& getLsObjectByName(const std::string& name){
        for(auto& pair : objects){
            if(pair.second.objectName == name)
                return pair.second;
        }
        throw std::runtime_error("getLsObjectByName failed for name"+name+"\n");
    }

    LsModifier& getLsModifierByID(unsigned int ID){
        auto it = modifiers.find(ID);
        if(it != modifiers.end())
            return modifiers[ID];
        throw std::runtime_error("getLsModifierByID failed for ID"+std::to_string(ID)+"\n");
    }

    LsModifier& getLsModifierByName(const std::string& name){
        for(auto& pair : modifiers){
            if(pair.second.modifierName == name)
                return pair.second;
        }
        throw std::runtime_error("getLsModifierByName failed for name"+name+"\n");
    }

    std::vector<HeliosPoint> requestFrame(float timestamp, const std::unordered_set<unsigned int>& objectsIDs = {}){
        std::vector<HeliosPoint> points;
        for(auto& it : modifiers){
            it.second.runForAllObjectsAt(timestamp);
        }
        if(objectsIDs.empty()){
            for(auto& it : objects){
                it.second.playEffect(timestamp);
                it.second.draw(points);
            }
        }
        else{
            for(unsigned int objectID : objectsIDs){
                auto it = objects.find(objectID);
                if(it != objects.end()){
                    it->second.playEffect(timestamp);
                    it->second.draw(points);
                }
            }
        }
        return points;
    }

    bool saveProject(){
        std::fstream file;
        file.open(projectsDir+name+".LSPROJ", std::ios::out);
        if(!file.is_open())
            return false;

        file << dateOfCreation << " " << length << " " << numOfLayers << "\n";
        //LsObjects
        file << objects.size() << "\n";
        for(auto& objEntry : objects){
            LsObject& obj = objEntry.second;
            file << obj.objectName << "\n";
            file << obj.effectName << "\n";
            file << obj.layer << " " << obj.startTime << " " << obj.duration << " " << obj.isInsideContainer << "\n";

            file << obj.colors.size() << "\n";
            for(const sf::Color& color : obj.colors){
                file << static_cast<int>(color.r) << " " << static_cast<int>(color.g) << " " << static_cast<int>(color.b) << "\n";
            }

            file << obj.getAllProperties().size() << "\n";
            for(auto& propEntry : obj.getAllProperties()){
                LsProperty& prop = propEntry.second;
                std::string name = propEntry.first;
                file << name << "\n";

                switch(prop.typeIndex){
                    case 0: file << prop.getConfig<bool>(); break;
                    case 1: file << prop.getConfig<int>(); break;
                    case 2: file << prop.getConfig<float>();
                }
                file << "\n";
            }
            file << objEntry.first << "\n";
        }
        //LsModifiers
        file << modifiers.size() << "\n";
        for(auto& modEntry : modifiers){
            const LsModifier& mod = modEntry.second;
            file << mod.modifierName << "\n";
            file << mod.linkedPropertyName << "\n";
            file << mod.layer << " " << mod.startTime << " " << mod.duration << " "
                  << static_cast<int>(mod.mode) << " " << static_cast<int>(mod.type) << "\n";

            file << mod.startVal.index() << "\n";
            switch(mod.startVal.index()){
                case 0: file << std::get<bool>(mod.startVal) << " " << std::get<bool>(mod.endVal); break;
                case 1: file << std::get<int>(mod.startVal) << " " << std::get<int>(mod.endVal); break;
                case 2: file << std::get<float>(mod.startVal) << " " << std::get<float>(mod.endVal);
            }
            file << "\n";

            file << mod.linkedObjectsIDs.size() << "\n";
            for(unsigned int ID : mod.linkedObjectsIDs){
                file << ID << " ";
            }
            file << "\n";

            file << modEntry.first << "\n";
        }

        file.close();
        return true;
    }

    std::string getlineStripped(std::fstream& stream){
        std::string temp, temp2;
        std::getline(stream, temp);
        if(!temp.empty() && temp.back() == '\r') temp.pop_back();
        return temp;
    }

    bool loadProject(){
        std::fstream file;
        file.open(projectsDir+name+".LSPROJ", std::ios::in);
        if(!file.is_open())
            return false;
        int c1;
        int c2;

        file >> dateOfCreation >> length >> numOfLayers;
        // LsObjects
        file >> c1;
        for(int i = 0; i < c1; i++){
            LsObject readO;
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            readO.objectName = getlineStripped(file);
            readO.effectName = getlineStripped(file);
            file >> readO.layer >> readO.startTime >> readO.duration >> readO.isInsideContainer;
            readO.initializeEffect();
            readO.colors.clear();

            file >> c2;
            for(int i = 0; i < c2; i++){
                int r, g, b;
                file >> r >> g >> b;
                readO.colors.emplace_back(sf::Color(r,g,b));
            }

            file >> c2;
            for(int i = 0; i < c2; i++){
                std::string propertyName;
                file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                propertyName = getlineStripped(file);
                LsProperty& property = readO.getProperty(propertyName);

                switch(property.typeIndex){
                    case 0: { bool val; file >> val; property.setConfig(val); break; }
                    case 1: { int val; file >> val; property.setConfig(val); break; }
                    case 2: { float val; file >> val; property.setConfig(val); break; }
                }
            }
            unsigned int ID;
            file >> ID;
            objects[ID] = readO;
        }

        //LsModifiers
        file >> c1;
        for(int i = 0; i < c1; i++){
            LsModifier readM;
            readM.projectBelonging = this;
            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            readM.modifierName = getlineStripped(file);
            readM.linkedPropertyName = getlineStripped(file);
            file >> readM.layer >> readM.startTime >> readM.duration;
            int mode, type;
            file >> mode >> type;
            readM.mode = static_cast<ModifierMode>(mode);
            readM.type = static_cast<ModifierType>(type);

            int typeIndex;
            file >> typeIndex;
            switch(typeIndex){
                case 0: { bool val; file >> val; readM.startVal = val; file >> val; readM.endVal = val; break; }
                case 1: { int val; file >> val; readM.startVal = val; file >> val; readM.endVal = val; break; }
                case 2: { float val; file >> val; readM.startVal = val; file >> val; readM.endVal = val; break; }
            }

            unsigned int ID;
            file >> c2;
            for(int i = 0; i < c2; i++){
                file >> ID;
                readM.link(ID);
            }

            file >> ID;
            modifiers[ID] = readM;
        }

        /*while(file){
            LsContainer readC;
        }*/

        file.close();
        return true;
    }

    Project(){
        dateOfCreation = getTodayString();
    }

    Project(std::string projectName, bool loadFromFile = false){
        dateOfCreation = getTodayString();
        name = projectName;
        if(loadFromFile)
            loadProject();
    }

    Project& operator=(const Project& other){
        length = other.length;
        name = other.name;
        dateOfCreation = other.dateOfCreation;
        numOfLayers = other.numOfLayers;
        curObjectID = other.curObjectID;
        objects = other.objects;
        modifiers = other.modifiers;
        containers = other.containers;
        audios = other.audios;

        for(auto& entry : modifiers)
            entry.second.projectBelonging = this;
        //gotta do this for containers too when I add those

        return *this;
    }
};

class WindowManager {
public:
    sf::RenderWindow window;
    sf::ContextSettings settings;
    unsigned int windowXsize;
    unsigned int windowYsize;
    unsigned int antialiasingLevel;
    bool fullscreen;

    void recreateWindow() {
        if (fullscreen)
            window.create(sf::VideoMode({windowXsize, windowYsize}), appName, sf::State::Fullscreen);
        else
            window.create(sf::VideoMode({windowXsize, windowYsize}), appName, sf::State::Windowed);
        window.setVerticalSyncEnabled(true);
    }

    WindowManager(){};
    WindowManager(unsigned int windowXsize, unsigned int windowYsize, bool fullscreen) {
        this->windowXsize = windowXsize;
        this->windowYsize = windowYsize;
        this->fullscreen = fullscreen;
        recreateWindow();
    }
};

class LasershowPlaybackManager{
private:
public:
    HeliosDac helios;
    bool devicesOpen = false;
    int numDevices = 0;
    unsigned int laserIndex = 0;
    uint8_t laserFlags = 0;
    unsigned int framerate = 60;

    float timestamp = 0.f;

    Project* loadedProject;
    std::vector<HeliosPoint> points;

    bool openMyLaser(){
        numDevices = helios.OpenDevicesOnlyUsb();
        if(numDevices <= 0){
            devicesOpen = true;
            return false;
        }
        else{
            devicesOpen = false;
            return true;
        }
    }

    void closeMyLaser(){
        devicesOpen = false;
        helios.Stop(laserIndex);
        helios.CloseDevices();
    }

    void drawWithLaser(){
        if(numDevices <= 0){
            openMyLaser();
        }
        if(numDevices > 0){
            while(helios.GetStatus(0) != 1){
                sf::sleep(sf::microseconds(100));
            }
        }

        if(numDevices > 0) {
            helios.WriteFrame(laserIndex, (framerate * points.size()), laserFlags, points.data(), points.size());
        }
    }

    void play(float dt){
        timestamp += dt;

        points = loadedProject->requestFrame(timestamp, {1, 2});
        for (int i = 0; i < points.size(); i++) {
            points[i].y = maxCord - points[i].y; // Y cordinate going from 0 at top to max at bottom is more intuitive
        }

        /*std::cout << "Points frame data: " << points.size() << std::endl;
        for(int i = 0; i < points.size(); i++){
            std::cout << "Point " << i << ":\n" << "Position - " << points[i].x << " " << points[i].y << "\n"
            << "Color - " << int(points[i].r) << " " << int(points[i].g) << " " << int(points[i].b) << "\n\n";
        }*/
    }

    ~LasershowPlaybackManager() {
        closeMyLaser();
    }
    LasershowPlaybackManager(Project& projectToUse){
        loadedProject = &projectToUse;
    }
};
