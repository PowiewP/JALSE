#pragma once
#include "utility.hpp"

class LsPreviewRenderer{
private:
    sf::RenderTexture canvas;
public:
    sf::Vector2u displaySize;
    sf::Vector2u displayPos;
    float pointWidth = 1.f;
    float timestamp = 0.f;

    LsProject* previewedProject;
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

    void loadNewProject(LsProject& previewedProject, std::unordered_set<unsigned int> previewedObjectsIDs = {}){
        this->previewedProject = &previewedProject;
        this->previewedObjectsIDs = previewedObjectsIDs;
    }

    LsPreviewRenderer(){}
    LsPreviewRenderer(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY, float pointWidth,
                     LsProject& previewedProject, std::unordered_set<unsigned int> previewedObjectsIDs = {}){
        canvas.resize({maxCord, maxCord});
        displaySize = {sizeX, sizeY};
        displayPos = {posX, posY};
        this->pointWidth = pointWidth;
        this->previewedProject = &previewedProject;
        this->previewedObjectsIDs = previewedObjectsIDs;
    }
};

class LsValueInput{
private:
    sf::RectangleShape field;
    sf::Text inputText = sf::Text(resources.getPickedFont(), "|", 1);
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
        centerTextInsideVertical(inputText, sf::Rect(inputPos, inputSize));
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

    LsValueInput(){}
    LsValueInput(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY,
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

        inputText.setFont(resources.getPickedFont());
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

class LsZipper {
private:
    sf::RectangleShape zip;
    sf::RectangleShape per;
    sf::Vector2i holdOffset;
public:
    sf::Vector2u perSize;
    sf::Vector2u perPos;
    sf::Vector2u zipSize;
    bool isVertical = false;
    bool isLocked = false;
    bool isHeld = false;

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

    float getZipFactor() {
        float edgeStart = perPos.x + (zipSize.x/2);
        float edgeEnd = perPos.x + perSize.x - (zipSize.x/2);
        float zipPos = zip.getPosition().x;
        if (isVertical) {
            edgeStart = perPos.y + (zipSize.y/2);
            edgeEnd = perPos.y + perSize.y - (zipSize.y/2);
            zipPos = zip.getPosition().y;
        }

        return ( (zipPos - edgeStart) / (edgeEnd - edgeStart) );
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
        if (!isHeld && !isLocked) {
            if (zip.getGlobalBounds().contains(static_cast<sf::Vector2f>(input.mousePos)) && input.mouseLeftJustPressed) {
                isHeld = true;
                holdOffset = {zip.getPosition().x - input.mousePos.x, zip.getPosition().y - input.mousePos.y};
                zip.setFillColor(sf::Color(std::clamp(zipColor.r-50, 0, 255), std::clamp(zipColor.g-50, 0, 255), std::clamp(zipColor.b-50, 0, 255)));
            }
        }
        if (isHeld && input.mouseLeftJustReleased) {
            isHeld = false;
            zip.setFillColor(zipColor);
        }

        if (isHeld) {
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

    LsZipper(){}
    LsZipper(unsigned int posX, unsigned int posY, unsigned int sizeX, unsigned int sizeY, std::variant<int, float> curVal, std::variant<int, float> startVal, std::variant<int, float> endVal, unsigned int zipSizeX = 0, unsigned int zipSizeY = 0, int outlineWidth = 4) {
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

class LsButton {
private:
    sf::RectangleShape button;
    std::variant<sf::Text, sf::Sprite> signature = sf::Text(resources.getPickedFont(), "Herobrine");
public:
    sf::Vector2u buttonSize;
    sf::Vector2u buttonPos;
    unsigned int signatureHeight;

    sf::Color backgroundColor = defaultBackgroundColor;
    sf::Color outlineColor = defaultOutlineColor;
    sf::Color textColor = defaultTextColor;
    int outlineWidth;

    bool isPressed = false;

    void tick(float dt, sf::RenderWindow& window) {
        if(sf::Rect(buttonPos, buttonSize).contains(input.mousePos) && input.mouseLeftJustPressed) {
            isPressed = true;
            button.setFillColor(sf::Color(std::clamp(backgroundColor.r-50, 0, 255), std::clamp(backgroundColor.g-50, 0, 255), std::clamp(backgroundColor.b-50, 0, 255)));
        }
        if(isPressed && input.mouseLeftJustReleased) {
            isPressed = false;
            button.setFillColor(backgroundColor);
        }
    }

    void setColors(const sf::Color& backgroundColor, const sf::Color& outlineColor, const sf::Color& signatureColor = sf::Color::White) {
        this->backgroundColor = backgroundColor;
        this->textColor = textColor;
        this->outlineColor = outlineColor;

        button.setFillColor(backgroundColor);
        if(std::holds_alternative<sf::Text>(signature))
            std::get<sf::Text>(signature).setFillColor(signatureColor);
        else
            std::get<sf::Sprite>(signature).setColor(signatureColor);
    }

    void draw(sf::RenderTarget& target) {
        target.draw(button);
        drawOutlineOf(button.getGlobalBounds(), outlineWidth, outlineColor, target);
        if(signature.index() == 0)
            target.draw(std::get<sf::Text>(signature));
        else
            target.draw(std::get<sf::Sprite>(signature));
    }

    LsButton(){}
    LsButton(sf::Vector2u buttonPos, sf::Vector2u buttonSize, std::string signature, unsigned int signatureHeight, int outlineWidth) {
        this->buttonSize = buttonSize;
        this->buttonPos = buttonPos;
        this->outlineWidth = outlineWidth;
        this->signatureHeight = signatureHeight;

        sf::Text temp = sf::Text(resources.getPickedFont(), signature, getFontSizeOfHeight(signatureHeight, resources.getPickedFont()));
        temp.setFillColor(textColor);
        centerTextInside(temp, sf::Rect(buttonPos, buttonSize));
        this->signature = temp;

        button.setPosition(sf::Vector2f(buttonPos.x, buttonPos.y));
        button.setSize(sf::Vector2f(buttonSize.x, buttonSize.y));
        button.setFillColor(backgroundColor);
    }
    LsButton(sf::Vector2u buttonPos, sf::Vector2u buttonSize, const sf::Texture& signature, unsigned int signatureHeight, int outlineWidth) {
        this->buttonSize = buttonSize;
        this->buttonPos = buttonPos;
        this->outlineWidth = outlineWidth;
        this->signatureHeight = signatureHeight;

        sf::Sprite temp = sf::Sprite(signature);
        if(temp.getGlobalBounds().size.y > signatureHeight) {
            temp.setScale({static_cast<float>(signatureHeight) / temp.getGlobalBounds().size.y, static_cast<float>(signatureHeight) / temp.getGlobalBounds().size.y});
        }
        temp.setOrigin(temp.getLocalBounds().getCenter());
        temp.setPosition(sf::Vector2f(buttonPos.x+(buttonSize.x/2), buttonPos.y+(buttonSize.y/2)));
        this->signature = temp;

        button.setPosition(sf::Vector2f(buttonPos.x, buttonPos.y));
        button.setSize(sf::Vector2f(buttonSize.x, buttonSize.y));
        button.setFillColor(backgroundColor);
    }
};

class ObjectEditPanel{
private:
    LsPreviewRenderer objectPreview;
    std::unordered_map<std::string, LsValueInput> inputs;
    std::vector<sf::Text> labels;
    sf::RectangleShape panel;
public:
    sf::Vector2u panelSize;
    sf::Vector2u panelPos;

    sf::Color backgroundColor;
    sf::Color outlineColor;
    int outlineWidth;

    int objectID;
    LsEffect* loadedObject;

    void tick(float dt, sf::RenderWindow& window){
        for(auto& it : inputs) {
            it.second.tick(dt, window);
            if(loadedObject->hasProperty(it.first))
                loadedObject->getProperty(it.first).setConfig(it.second.getLsValue());
        }
    }

    void generateRow(int rowNumber, std::string label, std::variant<int, float, std::string> var, bool disableInput = false){
        sf::Text newText = sf::Text(resources.getFont("Arial"), label, panelSize.y/23);
        newText.setFillColor(outlineColor);
        newText.setPosition({panelPos.x + (outlineWidth*4), panelPos.y + ((5.f+panelSize.y/25)*rowNumber)});

        labels.emplace_back(newText);
        inputs.emplace(std::pair<std::string, LsValueInput>(label, LsValueInput(panelPos.x + panelSize.x - (panelSize.x/2-(outlineWidth*5)), panelPos.y + ((5.f+panelSize.y/25)*rowNumber), panelSize.x/2-(outlineWidth*8), panelSize.y/25,
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
                     sf::Color backgroundColor, sf::Color outlineColor, int outlineWidth, LsEffect& object){
        panelSize = {sizeX, sizeY};
        panelPos = {posX, posY};
        this->backgroundColor = backgroundColor;
        this->outlineColor = outlineColor;
        this->outlineWidth = outlineWidth;
        this->loadedObject = &object;
        generatePanel();
    }
};

class LsTimelineBlock {
private:
    std::variant<LsEffect*, LsModifier*, LsContainer*> heldObject;

public:
    int objectTypeIndex = 0;

    LsTimelineBlock() = default;
};

class LsTimeline {
private:
    std::vector<sf::RectangleShape> layerBgRects;
    sf::RectangleShape topRow;
    std::vector<sf::RectangleShape> topRowTimeIndicators;
    std::vector<sf::Text> timeLabels;
    LsZipper verticalZipper;
    LsZipper horizontalZipper;

    sf::RenderTexture renderTexture;
    sf::View timelineView;
    sf::Sprite displaySprite = sf::Sprite(resources.getTexture("Error"));

    unsigned topRowHeight;
    unsigned layerHeight;
    unsigned horizontalZipperHeight;
    unsigned verticalZipperWidth;
    unsigned spacingBetweenTimeLabels = 5;

    static constexpr int defaultLayerCapacity = 5;
    static constexpr float topRowHeightRatio = 0.1f;
    static constexpr float zipperHeightRatio = 0.1f;
    static constexpr float textToTopRowHeightRatio = 0.6f;
public:
    sf::Vector2u timelineSize;
    sf::Vector2u timelinePos;

    LsProject* project;

    sf::Vector2u currentViewPos = {0, 0};
    unsigned pixelsPerSecond = 500;

    sf::Color backgroundColor = defaultBackgroundColor;
    sf::Color foregroundColor = defaultForegroundColor;
    sf::Color outlineColor = defaultOutlineColor;

    void reloadTimeline() {
        layerBgRects.clear();
        topRowTimeIndicators.clear();
        timeLabels.clear();

        //set useful values
        topRowHeight = static_cast<unsigned>(std::round(static_cast<float>(timelineSize.y)*topRowHeightRatio));
        horizontalZipperHeight = topRowHeight;
        unsigned modulo = (timelineSize.y - topRowHeight - horizontalZipperHeight) % defaultLayerCapacity;
        if(modulo != 0) {
            horizontalZipperHeight += modulo;
        }
        layerHeight = (timelineSize.y - topRowHeight - horizontalZipperHeight) / defaultLayerCapacity;
        verticalZipperWidth = horizontalZipperHeight;

        // top row background
        topRow = sf::RectangleShape(sf::Vector2f(9999999999999999.f, topRowHeight));
        topRow.setPosition(static_cast<sf::Vector2f>(timelinePos));
        topRow.setFillColor(foregroundColor);

        // layers backgrounds
        unsigned currentPosY = timelinePos.y + topRowHeight;
        for(int i = 0; i < project->numOfLayers; i++) {
            sf::RectangleShape temp = sf::RectangleShape(sf::Vector2f(9999999999999999.f, layerHeight));
            temp.setPosition(sf::Vector2f(timelinePos.x, currentPosY));
            temp.setFillColor(backgroundColor);

            currentPosY += layerHeight;

            layerBgRects.emplace_back(temp);
        }

        // top row time signatures
        unsigned textHeight = std::floor(static_cast<float>(topRowHeight) * textToTopRowHeightRatio);
        sf::Text test(resources.getPickedFont(), "000:00.00", getFontSizeOfHeight(textHeight, resources.getPickedFont()));
        unsigned textWidth = test.getGlobalBounds().size.x;

        unsigned minHundredths = static_cast<unsigned>(
        std::ceil((textWidth + spacingBetweenTimeLabels) / static_cast<float>(pixelsPerSecond) * 100.f));
        unsigned hundredths = ((minHundredths + 24) / 25) * 25; // round up to nearest multiple of 0.25s
        float secondsInterval = hundredths / 100.f;

        float spaceBetweenIntervals = static_cast<float>(pixelsPerSecond)*secondsInterval;
        unsigned numIntervals = static_cast<unsigned>(project->length / secondsInterval);
        for(int i = 1; i < numIntervals; i++) {
            float currentPosX = i * spaceBetweenIntervals;

            sf::RectangleShape temp = sf::RectangleShape(sf::Vector2f(5.f, topRowHeight - textHeight));
            temp.setOrigin({temp.getGlobalBounds().size.x/2, temp.getGlobalBounds().size.y});
            temp.setPosition({std::round(timelinePos.x + currentPosX), timelinePos.y + topRowHeight});
            temp.setFillColor(backgroundColor);

            topRowTimeIndicators.emplace_back(temp);

            sf::Text tempT = sf::Text(resources.getPickedFont(), getTimeString(i * secondsInterval), getFontSizeOfHeight(textHeight, resources.getPickedFont()));
            centerTextOrigin(tempT);
            tempT.setPosition(sf::Vector2f(timelinePos.x + currentPosX, timelinePos.y + (textHeight/2)));
            tempT.setFillColor(backgroundColor);

            timeLabels.emplace_back(tempT);
        }

        verticalZipper = LsZipper(timelinePos.x + timelineSize.x - verticalZipperWidth, timelinePos.y,
            verticalZipperWidth, timelineSize.y - horizontalZipperHeight,
            0, 0, 1000, 0, 0, 4);

        horizontalZipper = LsZipper(timelinePos.x, timelinePos.y + timelineSize.y - horizontalZipperHeight,
            timelineSize.x - verticalZipperWidth, horizontalZipperHeight,
            static_cast<int>(currentViewPos.x), 0, static_cast<int>(pixelsPerSecond * (project->length - (static_cast<float>(timelineSize.x)/static_cast<float>(pixelsPerSecond)))),
            0, 0, 2);

    }

    void setZoom(unsigned pixelsPerSecond) {
        float currentViewPosFactor = horizontalZipper.getZipFactor();

        this->pixelsPerSecond = pixelsPerSecond;

        reloadTimeline();

        horizontalZipper.setValue(static_cast<int>(currentViewPosFactor * pixelsPerSecond * (project->length - (static_cast<float>(timelineSize.x)/static_cast<float>(pixelsPerSecond)))));
    }

    void tick(float dt, sf::RenderWindow& window) {
        verticalZipper.tick(dt, window);
        horizontalZipper.tick(dt, window);

        //horizontalZipper.resizeZip(((static_cast<float>(timelineSize.x/pixelsPerSecond)/(static_cast<float>(pixelsPerSecond)*project->length)))*timelineSize.x, horizontalZipperHeight);
        // to do

        currentViewPos.x = horizontalZipper.getValue<int>();
        currentViewPos.y = verticalZipper.getValue<int>();
    }

    void draw(sf::RenderTarget& target) {
        renderTexture.clear(sf::Color::Transparent);

        timelineView.setCenter(sf::Vector2f(timelinePos.x + (timelineSize.x/2) + currentViewPos.x, timelinePos.y + (timelineSize.y/2) + currentViewPos.y));
        renderTexture.setView(timelineView);

        for(int i = 0; i < layerBgRects.size(); i++) {
            renderTexture.draw(layerBgRects[i]);
            drawOutlineOf(layerBgRects[i].getGlobalBounds(), 2.f, outlineColor, renderTexture);
        }

        timelineView.setCenter(sf::Vector2f(timelinePos.x + (timelineSize.x/2) + currentViewPos.x, timelinePos.y + (timelineSize.y/2)));
        renderTexture.setView(timelineView);

        renderTexture.draw(topRow);
        for(int i = 0; i < topRowTimeIndicators.size(); i++) {
            renderTexture.draw(topRowTimeIndicators[i]);
        }
        for(int i = 0; i < timeLabels.size(); i++) {
            renderTexture.draw(timeLabels[i]);
        }

        renderTexture.display();
        displaySprite.setTexture(renderTexture.getTexture(), true);
        displaySprite.setPosition(sf::Vector2f(timelinePos));
        target.draw(displaySprite);

        verticalZipper.draw(target);
        horizontalZipper.draw(target);
    }

    LsTimeline() = default;
    LsTimeline(sf::Vector2u timelinePos, sf::Vector2u timelineSize, LsProject& project) {
        this->timelineSize = timelineSize;
        this->timelinePos = timelinePos;
        this->project = &project;

        reloadTimeline();

        renderTexture.resize(timelineSize);
        timelineView.setSize(static_cast<sf::Vector2f>(timelineSize));
        timelineView.setCenter(sf::Vector2f(timelinePos.x + (timelineSize.x/2), timelinePos.y + (timelineSize.y/2)));
        renderTexture.setView(timelineView);
    }
};