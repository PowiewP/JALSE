#pragma once
#include "utility.hpp"

class PreviewRenderer{
private:
    sf::RenderTexture canvas;
public:
    sf::Vector2u displaySize;
    sf::Vector2u displayPos;
    float pointWidth = 1.f;
    float timestamp = 0.f;

    Project* previewedProject;
    std::unordered_set<unsigned int> previewedObjectsIDs;

    std::vector<HeliosPoint> points;

    void interpretPoints(){
        if(points.size() == 0)
            return;

        sf::RectangleShape trail;

        for(int i = 0; i < points.size()-1; i++){
            HeliosPoint& curPoint = points[i];
            HeliosPoint& nextPoint = points[i+1];

            sf::Vector2f curPointPos = {curPoint.x, curPoint.y};
            sf::Vector2f nextPointPos = {nextPoint.x, nextPoint.y};

            float length = distanceBetween(curPointPos, nextPointPos);
            if(length < pointWidth)
                length = pointWidth;

            trail.setSize({length, pointWidth});
            trail.setOrigin({0.f, pointWidth/2.f});
            trail.setRotation(sf::degrees(angleDegreesBetween(curPointPos, nextPointPos)));
            trail.setPosition(curPointPos);
            trail.setFillColor(sf::Color(nextPoint.r, nextPoint.g, nextPoint.b));

            if(trail.getFillColor() != sf::Color::Black)
                canvas.draw(trail);
        }
    }

    void setTimestamp(float seconds){
        timestamp = seconds;

        canvas.clear();

        points = previewedProject->requestFrame(timestamp, previewedObjectsIDs);
        interpretPoints();

        canvas.display();
    }

    void play(float dt){
        timestamp += dt;

        canvas.clear();

        points = previewedProject->requestFrame(timestamp, previewedObjectsIDs);
        interpretPoints();

        canvas.display();
    }

    void draw(sf::RenderTarget& target){
        sf::Sprite displaySprite(canvas.getTexture());
        displaySprite.setScale({static_cast<float>(displaySize.x)/canvas.getSize().x, static_cast<float>(displaySize.y)/canvas.getSize().y});
        displaySprite.setPosition(static_cast<sf::Vector2f>(displayPos));

        target.draw(displaySprite);
    }

    void loadNewProject(Project& previewedProject, std::unordered_set<unsigned int> previewedObjectsIDs = {}){
        this->previewedProject = &previewedProject;
        this->previewedObjectsIDs = previewedObjectsIDs;
    }

    PreviewRenderer(){}
    PreviewRenderer(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY, float pointWidth,
                     Project& previewedProject, std::unordered_set<unsigned int> previewedObjectsIDs = {}){
        canvas.resize({maxCord, maxCord});
        displaySize = {sizeX, sizeY};
        displayPos = {posX, posY};
        this->pointWidth = pointWidth;
        this->previewedProject = &previewedProject;
        this->previewedObjectsIDs = previewedObjectsIDs;
    }
};

class ValueInput{
private:
    sf::RectangleShape field;
    sf::Text inputText = sf::Text(usedFont, "|", 1);
    std::string inputString;
    sf::RenderTexture canvas;
    sf::View canvasView;

    sf::RectangleShape cursor;
    bool cursorVisible = false;
    float cursorBlinkTimer = 0.f;
    int cursorIndexPos;
    float maxViewPos;
public:
    sf::Vector2u inputSize;
    sf::Vector2u inputPos;

    sf::Color backgroundColor;
    sf::Color outlineColor;
    sf::Color textColor;
    int outlineWidth;
    int textPadding = 10;
    float cursorBlinkTime = 0.3f;

    bool isFocused = false;
    bool isDisabled = false;

    std::variant<int, float, std::string> value;

    LsValue getLsValue(){
        if(std::holds_alternative<std::string>(value)){
            throw std::runtime_error("Attempted to get LsValue from a valueInput containing a string value (only ints and floats allowed)");
        }
        else{
            LsValue temp;
            if(std::holds_alternative<float>(value))
                temp = std::get<float>(value);
            else
                temp = std::get<int>(value);
            return temp;
        }
    }

    std::string valueToInput(){
        if(std::holds_alternative<std::string>(value))
            return std::get<std::string>(value);
        else if(std::holds_alternative<float>(value))
            return std::to_string(std::get<float>(value));
        else
            return std::to_string(std::get<int>(value));
    }

    std::variant<int, float, std::string> inputToValue(){
        if(std::holds_alternative<std::string>(value))
            return inputString;
        if(std::holds_alternative<float>(value)){
            if(inputString.length() > 0)
                return std::stof(inputString);
            else
                return 0.f;
        }
        if(std::holds_alternative<int>(value)){
            if(inputString.length() > 0)
                return std::stoi(inputString);
            else
                return 0;
        }
    }

    void resolveInput(){
        // cursor pos from click
        if(input.mouseLeftJustPressed && field.getGlobalBounds().contains(static_cast<sf::Vector2f>(input.mousePos))){
            std::vector<float> axisXdistanceToChars(inputString.length()+1);
            for(int i = 0; i <= inputString.length(); i++){
                axisXdistanceToChars[i] = distanceBetween(static_cast<sf::Vector2f>(input.mousePos), inputText.findCharacterPos(i));
            }
            int minDistanceIndex = 0;
            for(int i = 0; i < axisXdistanceToChars.size(); i++){
                if(axisXdistanceToChars[i] < axisXdistanceToChars[minDistanceIndex])
                    minDistanceIndex = i;
            }
            cursorIndexPos = minDistanceIndex;
        }

        // arrows and backspace
        if(input.keyJustPressed[sf::Keyboard::Scancode::Left] && cursorIndexPos > 0)
            cursorIndexPos--;
        if(input.keyJustPressed[sf::Keyboard::Scancode::Right] && cursorIndexPos < inputString.length())
            cursorIndexPos++;
        if(input.keyJustPressed[sf::Keyboard::Scancode::Backspace] && cursorIndexPos != 0){
            inputString.erase(cursorIndexPos-1, 1);
            cursorIndexPos--;
        }

        // text input
        inputString.insert(cursorIndexPos, input.textEntered);
        cursorIndexPos += input.textEntered.length();

        value = inputToValue();
    }

    void tick(float dt, sf::RenderWindow& window){

        if(!isFocused && input.mouseLeftJustPressed && field.getGlobalBounds().contains(static_cast<sf::Vector2f>(input.mousePos)) && !isDisabled){
            isFocused = true;
        }
        else{
            if((input.mouseLeftJustPressed && !field.getGlobalBounds().contains(static_cast<sf::Vector2f>(input.mousePos)))
            || input.keyIsHeld[sf::Keyboard::Scancode::Escape]){
                isFocused = false;
                if(inputString.size() <= 0 && !std::holds_alternative<std::string>(value))
                    inputString = "0";
            }
        }

        if(isFocused){
            cursorBlinkTimer += dt;
            if(cursorBlinkTimer >= cursorBlinkTime){
                cursorBlinkTimer -= cursorBlinkTime;
                cursorVisible = !cursorVisible;
            }

            resolveInput();
        }
        else{
            inputString = valueToInput();
        }

        inputText.setString(inputString);
        inputText.setOrigin({inputText.getGlobalBounds().size.x + inputText.getLocalBounds().position.x,
                             0.f});
        inputText.setPosition(sf::Vector2f(inputPos.x + inputSize.x - textPadding - cursor.getSize().x, 0.f));
        centerTextVertical(inputText, sf::Rect(inputPos, inputSize));
        cursor.setPosition({inputText.findCharacterPos(cursorIndexPos).x,
                            inputPos.y + (inputSize.y/2)});
    }

    void draw(sf::RenderTarget& target){
        target.draw(field);

        float apart = cursor.getPosition().x - canvasView.getCenter().x;
        if(apart < -canvasView.getSize().x/2){
            canvasView.setCenter({canvasView.getCenter().x + (apart + canvasView.getSize().x/2), canvasView.getCenter().y});
        }
        if(apart > canvasView.getSize().x/2){
            canvasView.setCenter({canvasView.getCenter().x + (apart - canvasView.getSize().x/2) + cursor.getSize().x, canvasView.getCenter().y});
        }
        if(cursorIndexPos == inputString.length()){
            canvasView.setCenter(static_cast<sf::Vector2f>(inputPos)+(static_cast<sf::Vector2f>(inputSize)/2.f));
        }

        canvas.clear(sf::Color::Transparent);
        canvas.setView(canvasView);
        canvas.draw(inputText);
        if(isFocused && cursorVisible)
           canvas.draw(cursor);
        canvas.display();
        sf::Sprite canvasDisplay(canvas.getTexture());
        canvasDisplay.setPosition(static_cast<sf::Vector2f>(inputPos));

        target.draw(canvasDisplay);

        drawOutlineOf(field.getGlobalBounds(), outlineWidth, outlineColor, target);
    }

    ValueInput(){}
    ValueInput(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY,
                sf::Color backgroundColor, sf::Color outlineColor, int outlineWidth, sf::Color textColor, std::variant<int, float, std::string> value, bool isDisabled = false){
        inputPos = {posX, posY};
        inputSize = {sizeX, sizeY};
        this->backgroundColor = backgroundColor;
        this->outlineColor = outlineColor;
        this->outlineWidth = outlineWidth;
        this->textColor = textColor;
        this->isDisabled = isDisabled;

        this->value = value;

        field.setSize(static_cast<sf::Vector2f>(inputSize));
        field.setPosition(static_cast<sf::Vector2f>(inputPos));
        field.setFillColor(backgroundColor);

        inputText.setFont(usedFont);
        inputText.setCharacterSize(inputSize.y-textPadding);
        inputText.setFillColor(textColor);
        inputString = valueToInput();
        inputText.setOrigin({inputText.getGlobalBounds().size.x + inputText.getLocalBounds().position.x,
                             (inputText.getGlobalBounds().size.y / 2.f) + inputText.getLocalBounds().position.y});
        inputText.setPosition(sf::Vector2f(inputPos.x + inputSize.x - textPadding, inputPos.y + (inputSize.y/2)));

        canvas.resize(inputSize);
        canvasView.setSize(static_cast<sf::Vector2f>(inputSize));
        canvasView.setCenter(static_cast<sf::Vector2f>(inputPos)+(static_cast<sf::Vector2f>(inputSize)/2.f));
        canvas.setView(canvasView);

        cursor.setSize({inputText.getGlobalBounds().size.y/10.f, inputText.getGlobalBounds().size.y});
        cursor.setFillColor(textColor);
        cursor.setOrigin({0.f, cursor.getGlobalBounds().size.y/2});
        cursorIndexPos = inputString.length();
    }
};

class Zipper {
private:
    sf::RectangleShape zip;
    sf::RectangleShape per;
    bool zipHeld = false;
    sf::Vector2u holdOffset;
public:
    sf::Vector2u perSize;
    sf::Vector2u perPos;
    sf::Vector2u zipSize;
    bool isVertical = false;
    bool isLocked = false;

    sf::Color perColor = defaultBackgroundColor;
    sf::Color zipColor = defaultForegroundColor;
    sf::Color outlineColor = defaultOutlineColor;
    int outlineWidth;

    std::variant<int, float> startVal;
    std::variant<int, float> endVal;
    int typeIndex;

    void resizeZip(unsigned int sizeX, unsigned int sizeY) {
        unsigned int sizeLost = zipSize.x - sizeX;
        if (isVertical)
            sizeLost = zipSize.y - sizeY;

        zip.setSize(sf::Vector2f(sizeX, sizeY));
        zip.setPosition(sf::Vector2f(zip.getPosition().x + sizeLost, zip.getPosition().y));
        if (isVertical)
            zip.setPosition(sf::Vector2f(zip.getPosition().x, zip.getPosition().y + sizeLost));

        zipSize = {sizeX, sizeY};
    }

    void setValue(std::variant<int, float> val, std::optional<std::variant<int, float>> startVal = {},  std::optional<std::variant<int, float>> endVal = {}) {
        if (startVal.has_value()) {
            if (!endVal.has_value())
                throw std::invalid_argument("Use either 1 or 3 parameters when using Zipper::setValue! Provided 2");
            this->startVal = startVal.value();
            this->endVal = endVal.value();
            this->typeIndex = startVal.value().index();
        }
        else {
            if(val.index() != typeIndex) {
                throw std::invalid_argument("Zipper::setValue - 1 argument used, is not valid type. Type provided: "+std::to_string(val.index()));
            }
        }

        float t;
        if (this->startVal == this->endVal)
            t = 0.5f;
        else {
            if (typeIndex == 0) {
                float start = static_cast<float>(std::get<int>(this->startVal));
                float end = static_cast<float>(std::get<int>(this->endVal));
                t = (std::get<int>(val) - start) / (end - start);
            } else {
                float start = std::get<float>(this->startVal);
                float end = std::get<float>(this->endVal);
                t = (std::get<float>(val) - start) / (end - start);
            }
        }

        float edgeStart = perPos.x + (zipSize.x/2);
        float edgeEnd = perPos.x + perSize.x - (zipSize.x/2);
        zip.setPosition(sf::Vector2f(perPos.x+(zipSize.x/2) + ((edgeEnd - edgeStart) * t), perPos.y));
        if (isVertical) {
            edgeStart = perPos.y + (zipSize.y/2);
            edgeEnd = perPos.y + perSize.y - (zipSize.y/2);
            zip.setPosition(sf::Vector2f(perPos.x, perPos.y+(zipSize.y/2) + ((edgeEnd - edgeStart) * t)));
        }
    }

    template<typename T>
    T getValue() {
        T val;
        float edgeStart = perPos.x + (zipSize.x/2);
        float edgeEnd = perPos.x + perSize.x - (zipSize.x/2);
        float zipPos = zip.getPosition().x;
        if (isVertical) {
            edgeStart = perPos.y + (zipSize.y/2);
            edgeEnd = perPos.y + perSize.y - (zipSize.y/2);
            zipPos = zip.getPosition().y;
        }

        float t = ( (zipPos - edgeStart) / (edgeEnd - edgeStart) );
        if (typeIndex == 0)
            val = std::round(static_cast<float>(std::get<int>(startVal)) + t * static_cast<float>(std::get<int>(endVal) - std::get<int>(startVal)));
        else
            val = std::get<float>(startVal) + t * (std::get<float>(endVal) - std::get<float>(startVal));

        return val;
    }

    void tick(float dt, sf::RenderWindow& window) {
        if (!zipHeld && !isLocked) {
            if (zip.getGlobalBounds().contains(static_cast<sf::Vector2f>(input.mousePos)) && input.mouseLeftJustPressed) {
                zipHeld = true;
                holdOffset = {zip.getPosition().x - input.mousePos.x, zip.getPosition().y - input.mousePos.y};
            }
        }
        if (zipHeld) {
            if (input.mouseLeftJustReleased)
                zipHeld = false;
        }

        if (zipHeld) {
            if (isVertical) {
                zip.setPosition(sf::Vector2f(perPos.x, std::clamp(input.mousePos.y + holdOffset.y, perPos.y+(zipSize.y/2), perPos.y+perSize.y-(zipSize.y/2))));
            }
            else {
                zip.setPosition(sf::Vector2f(std::clamp(input.mousePos.x + holdOffset.x, perPos.x + (zipSize.x/2), perPos.x+perSize.x-(zipSize.x/2)), perPos.y));
            }
        }
    }

    void draw(sf::RenderTarget& target) {
        target.draw(per);
        drawOutlineOf(per.getGlobalBounds(), outlineWidth, outlineColor, target);
        target.draw(zip);
    }

    void setColors(const sf::Color& backgroundColor, const sf::Color& foregroundColor, const sf::Color& outlineColor) {
        this->perColor = backgroundColor;
        this->zipColor = foregroundColor;
        this->outlineColor = outlineColor;

        zip.setFillColor(zipColor);
        per.setFillColor(perColor);
    }

    Zipper(){}
    Zipper(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY, std::variant<int, float> curVal, std::variant<int, float> startVal, std::variant<int, float> endVal, unsigned int zipSizeX = 0, unsigned int zipSizeY = 0, int outlineWidth = 4) {
        this->perSize = sf::Vector2u(sizeX, sizeY);
        this->perPos = sf::Vector2u(posX, posY);
        this->outlineWidth = outlineWidth;

        if (sizeY > sizeX)
            isVertical = true;

        if (zipSizeX == 0 || zipSizeY == 0) {
            zipSize = sf::Vector2u(sizeX/10, sizeY);
            if (isVertical)
                zipSize = sf::Vector2u(sizeX, sizeY/10);
        }
        else
            zipSize = sf::Vector2u(zipSizeX, zipSizeY);

        per.setPosition(sf::Vector2f(posX, posY));
        per.setSize(sf::Vector2f(sizeX, sizeY));
        per.setFillColor(perColor);

        zip.setSize(sf::Vector2f(zipSize.x, zipSize.y));
        zip.setOrigin({(zipSize.x/2), 0});
        if (isVertical)
            zip.setOrigin({0, (zipSize.x/2)});
        zip.setFillColor(zipColor);

        setValue(curVal, startVal, endVal);
    }
};

class Button {
private:
    sf::RectangleShape button;
    std::variant<sf::Text, sf::Sprite> signature = sf::Text(usedFont, "Herobrine");
public:
    sf::Vector2u buttonSize;
    sf::Vector2u buttonPos;
    unsigned int signatureHeight;

    sf::Color backgroundColor = defaultBackgroundColor;
    sf::Color outlineColor = defaultOutlineColor;
    sf::Color textColor = defaultTextColor;
    int outlineWidth;

    bool isPressed = false;

    void draw(sf::RenderTarget& target) {
        target.draw(button);
        drawOutlineOf(button.getGlobalBounds(), outlineWidth, outlineColor, target);
        if(signature.index() == 0)
            target.draw(std::get<sf::Text>(signature));
        else
            target.draw(std::get<sf::Sprite>(signature));
    }

    Button(){}
    Button(sf::Vector2u buttonSize, sf::Vector2u buttonPos, std::variant<std::string, sf::Texture> signature, unsigned int signatureHeight, int outlineWidth) {
        this->buttonSize = buttonSize;
        this->buttonPos = buttonPos;
        this->outlineWidth = outlineWidth;
        this->signatureHeight = signatureHeight;
        if(std::holds_alternative<std::string>(signature)) {
            sf::Text temp = sf::Text(usedFont, std::get<std::string>(signature), getFontSizeOfHeight(signatureHeight));
            temp.setFillColor(textColor);
            centerText(temp, sf::Rect(buttonPos, buttonSize));
            this->signature = temp;
        }

        button.setPosition(sf::Vector2f(buttonPos.x, buttonPos.y));
        button.setSize(sf::Vector2f(buttonSize.x, buttonSize.y));
        button.setFillColor(backgroundColor);
    }
};

class ObjectEditPanel{
private:
    PreviewRenderer objectPreview;
    std::unordered_map<std::string, ValueInput> inputs;
    std::vector<sf::Text> labels;
    sf::RectangleShape panel;
public:
    sf::Vector2u panelSize;
    sf::Vector2u panelPos;

    sf::Color backgroundColor;
    sf::Color outlineColor;
    int outlineWidth;

    int objectID;
    LsObject* loadedObject;

    void tick(float dt, sf::RenderWindow& window){
        for(auto& it : inputs) {
            it.second.tick(dt, window);
            if(loadedObject->hasProperty(it.first))
                loadedObject->getProperty(it.first).setConfig(it.second.getLsValue());
        }
    }

    void generateRow(int rowNumber, std::string label, std::variant<int, float, std::string> var, bool disableInput = false){
        sf::Text newText = sf::Text(usedFont, label, panelSize.y/23);
        newText.setFillColor(outlineColor);
        newText.setPosition({panelPos.x + (outlineWidth*4), panelPos.y + ((5.f+panelSize.y/25)*rowNumber)});

        labels.emplace_back(newText);
        inputs.emplace(std::pair<std::string, ValueInput>(label, ValueInput(panelPos.x + panelSize.x - (panelSize.x/2-(outlineWidth*5)), panelPos.y + ((5.f+panelSize.y/25)*rowNumber), panelSize.x/2-(outlineWidth*8), panelSize.y/25,
                               sf::Color(180,180,180), sf::Color(100,100,100), 2, sf::Color(10,10,10), var, disableInput)));
    }

    void generatePanel(){
        panel.setSize(static_cast<sf::Vector2f>(panelSize));
        panel.setPosition(static_cast<sf::Vector2f>(panelPos));
        int i = 1;

        generateRow(i, "Effect Name: ", loadedObject->effectName, true);
        i++;
        generateRow(i, "Object Name: ", loadedObject->objectName);
        i += 2;
        generateRow(i, "Start Time: ", loadedObject->startTime);
        i++;
        generateRow(i, "Duration: ", loadedObject->duration);
        i += 2;

        std::unordered_map<std::string, LsProperty>& properties = loadedObject->getAllProperties();
        for(auto& it : properties){
            LsProperty& property = it.second;
            const std::string& name = it.first;

            if(property.isInternal || property.typeIndex == 0)
                continue;
            if(property.typeIndex == 1){
                generateRow(i, name, property.getConfig<int>());
            }
            if(property.typeIndex == 2){
                generateRow(i, name, property.getConfig<float>());
            }
            i++;
        }
    }

    void draw(sf::RenderTarget& target){
        target.draw(panel);
        drawOutlineOf(panel.getGlobalBounds(), outlineWidth, outlineColor, target);
        for(auto& it : labels){
            target.draw(it);
        }
        for(auto& it : inputs){
            it.second.draw(target);
        }
        objectPreview.draw(target);
    }

    ObjectEditPanel(){}
    ObjectEditPanel(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY,
                     sf::Color backgroundColor, sf::Color outlineColor, int outlineWidth, LsObject& object){
        panelSize = {sizeX, sizeY};
        panelPos = {posX, posY};
        this->backgroundColor = backgroundColor;
        this->outlineColor = outlineColor;
        this->outlineWidth = outlineWidth;
        this->loadedObject = &object;
        generatePanel();
    }
};

class Timeline {
private:
    std::vector<sf::RectangleShape> bgRects;
    std::vector<sf::RectangleShape> topRowRects;
    std::vector<sf::Text> timeLabels;
public:
    sf::Vector2u timelineSize;
    sf::Vector2u timelinePos;

    sf::Color backgroundColor;
    sf::Color foregroundColor;
};