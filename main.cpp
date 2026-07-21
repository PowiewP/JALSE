#include <iostream>
#include <cmath>
#include <thread>
#include <conio.h>
#include <SFML/Graphics.hpp>
#include "HeliosDac.h"

#include "classes.hpp"
#include "LsObject.hpp"
#include "utility.hpp"
#include "globals.hpp"
#include "UI.hpp"

WindowManager windowManager;

sf::Clock deltaClock;
float dt = 0.f;
float timer = 0.f;

LsPreviewRenderer prevRend;
LsProject proj;

LsValueInput valueInput1;
LsValueInput valueInput2;
LsValueInput valueInput3;

ObjectEditPanel panel;

LsEffect obj2;
unsigned int ID = 1;

LsZipper zipper;
LsButton button;

LsTimeline timeline;

void mainLoop(){
    LasershowPlaybackManager lsPlayback(proj);

    while(running){
        dt = deltaClock.getElapsedTime().asSeconds();
        deltaClock.restart();
        timer += dt;

        input.handleEvents(windowManager.window);

        if(input.keyIsHeld[sf::Keyboard::Scancode::Escape] || !windowManager.window.isOpen()){
            running = false;
        }

        lsPlayback.play(dt);
        prevRend.play(dt);

        //valueInput1.tick(dt, windowManager.window);
        //valueInput2.tick(dt, windowManager.window);
        //valueInput3.tick(dt, windowManager.window);

        //proj.getLsObjectByID(ID).getProperty("circleRadiusX").setConfig(valueInput1.getLsValue());
        //proj.getLsObjectByID(ID).getProperty("numOfDots").setConfig(valueInput2.getLsValue());

        if(input.keyIsHeld[sf::Keyboard::Scancode::D]){
            prevRend.setTimestamp(0.f);
            lsPlayback.timestamp = 0.f;
        }
        if(input.keyIsHeld[sf::Keyboard::Scancode::S]){
            prevRend.setTimestamp(prevRend.timestamp - dt);
            lsPlayback.timestamp -= dt;
        }
        if(input.keyIsHeld[sf::Keyboard::Scancode::A]){
            prevRend.setTimestamp(prevRend.timestamp - (2*dt));
            lsPlayback.timestamp -= 2*dt;
        }
        if(input.keyJustPressed[sf::Keyboard::Scancode::Backslash]){
            if(ID == 1){
                ID++;
                panel = ObjectEditPanel(1000, 100, 700, 800, sf::Color(230,230,230), sf::Color(100,100,100), 5, proj.getLsEffectByID(ID));
            }
            else{
                ID--;
                panel = ObjectEditPanel(1000, 100, 700, 800, sf::Color(230,230,230), sf::Color(100,100,100), 5, proj.getLsEffectByID(ID));
            }
        }

        panel.tick(dt, windowManager.window);
        zipper.tick(dt, windowManager.window);
        button.tick(dt, windowManager.window);
        timeline.tick(dt, windowManager.window);

        if(zipper.isHeld) {
            timeline.setZoom(zipper.getValue<int>());
        }

        windowManager.window.clear(backgroundColor);
        prevRend.draw(windowManager.window);
        //valueInput1.draw(windowManager.window);
        //valueInput2.draw(windowManager.window);
        //valueInput3.draw(windowManager.window);
        panel.draw(windowManager.window);
        zipper.draw(windowManager.window);
        button.draw(windowManager.window);
        timeline.draw(windowManager.window);
        windowManager.window.display();
        lsPlayback.drawWithLaser();
    }
}

int main(){
    windowManager = WindowManager(2520, 1280, false);

    obj2 = LsEffect("Circling Dots", "real existing object", 0.f, 1000.f, 0);
    //obj2.getProperty("Dots Count").setConfig(32);
    //obj2.getProperty("Moving Clockwise").setConfig(false);
    ID = proj.emplaceLsEffect(obj2);

    //obj2 = LsObject("Triangle", "tri", 0.f, 1000.f, 0);
    //proj.emplaceLsObject(obj2);

    //LsModifier mod = LsModifier("Circle Scale X", 1.f, 0.1f, 1.f, 0.f, SET, EASEINOUTSINE, 5000.f, true);
    //mod.link(ID);
    //proj.emplaceLsModifier(mod);

    /*obj2 = LsObject("Triangle", "tria", 0.f, 1000.f, 0);
    ID = proj.emplaceLsObject(obj2);

    LsModifier mod = LsModifier("Rotation", 0.f, 1.f, 0.f, 360.f, SET, LINEAR, 100, false);
    mod.link(ID);
    proj.emplaceLsModifier(mod);*/

    proj.name = "New Project";
    //proj.loadProject();
    proj.saveProject();

    prevRend = LsPreviewRenderer(100, 100, 819, 819, 30, proj);

    /*valueInput1 = ValueInput(700, 100, 300, 50, sf::Color(230,230,230), sf::Color(100,100,100), 5, sf::Color(80,8,8), proj.getLsObjectByID(1).getProperty("circleRadiusX").getConfig<float>());
    valueInput2 = ValueInput(700, 300, 400, 60, sf::Color(230,230,230), sf::Color(100,100,100), 5, sf::Color(8,8,8), proj.getLsObjectByID(1).getProperty("numOfDots").getConfig<int>());
    valueInput3 = ValueInput(700, 600, 400, 70, sf::Color(230,230,230), sf::Color(100,100,100), 5, sf::Color(8,8,8), "sup");*/

    resources.pickFont("Arial");

    panel = ObjectEditPanel(1000, 100, 700, 800, sf::Color(230,230,230), sf::Color(100,100,100), 5, proj.getLsEffectByID(ID));

    zipper = LsZipper(20, 20, 1000, 50, 500, 50, 1000);
    zipper.setColors({100, 0, 0}, {200, 0, 0}, {50, 0, 0});

    button = LsButton({1300, 750}, {200, 200}, resources.getTexture("nigger"), 150, 4);
    button.setColors({150, 0, 0}, {100, 0, 0});

    timeline = LsTimeline({100, 700}, {1500, 500}, proj);

    deltaClock.restart();
    mainLoop();
return 0;
}
