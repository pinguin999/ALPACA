#include "dialog_manager.hpp"
#include "../game.hpp"
#include <filesystem>

#define BOX_HEIGHT 90

DialogManager::DialogManager(std::shared_ptr<Game> game)
    : typewriterFont("fonts/BarcadeBrawlRegular-cc0.ttf", 30),
      dialogFont("fonts/ABeeZee-Regular.ttf", 25),
      currentTypewriterLine(typewriterFont, ""),
      currentNarratorText(""),
      dialog_callback((*game->lua_state)["pass"]),
      game(game)
{
    isNarratorTextVisible = false;
    currentNarratorText.setFont(dialogFont);
    currentNarratorText.setAlign(jngl::Alignment::CENTER);
    bubble = nullptr;
    selected_index = -1;
}

void DialogManager::loadDialogsFromFile(std::string fileName, bool initializeVariables)
{
    if (auto _game = game.lock())
    {
        schnackFile = schnacker::SchnackFile::loadFromString(_game->lua_state, jngl::readAsset(fileName).str(), initializeVariables);
        schnackFile->setCurrentLocale(_game->language);
    }
}

void DialogManager::play(std::string characterName, jngl::Vec2, sol::function callback)
{
    cancelDialog();

    currentDialog = schnackFile->dialogs[characterName];
    if (currentDialog)
    {
        currentNode = currentDialog->getEntryNode();
        continueCurrent();
        dialog_callback = callback;
    }else
    {
        jngl::debugLn("Dialog " + characterName + " not found.");
    }
}

void DialogManager::step()
{
    if(!currentDialog)
    {
        wasActiveLastFrame = false;
        return;
    }
    if (auto _game = game.lock())
    {
        auto pointer_position = _game->pointer->getPosition();

        size_t pos = BOX_HEIGHT * (choiceTexts.size() -1);
        int temp_index = 0;
        for (auto text = choiceTexts.begin(); text != choiceTexts.end(); text++)
        {
            if (pointer_position.y > 1080/2 - BOX_HEIGHT - pos && pointer_position.y < 1080/2 - BOX_HEIGHT - pos + BOX_HEIGHT)
            {
                selected_index = temp_index;
                break;
            }
            temp_index++;
            pos -= BOX_HEIGHT;
        }

        if(selected_index >= 0)
        {
            auto direction = _game->pointer->getMovementStep();
            if (direction == jngl::Vec2(0, 1))
            {
                selected_index = std::max((int)selected_index - 1, 0);
            }

            if (direction == jngl::Vec2(0, -1))
            {
                selected_index = std::min((int)selected_index + 1, (int)choiceTexts.size() - 1);
            }
        }

        if(_game->pointer->primaryPressed() && !_game->pointer->isPrimaryAlreadyHandled())
        {
            if (!wasActiveLastFrame && isActive())
            {
                if (selected_index != -1)
                {
                    selectCurrentAnswer();
                    _game->pointer->setPrimaryHandled();
                }
                else
                {
                    continueCurrent();
                    _game->pointer->setPrimaryHandled();
                }
            }
            wasActiveLastFrame = true;
        }
        else
        {
            wasActiveLastFrame = false;
        }

        if (bubble != nullptr)
        {
            bubble->step();
        }

        if (currentTypewriterProgress >= 0 && currentTypewriterProgress < currentTypewriterText.length())
        {
            int currentProgress = (int)currentTypewriterProgress;
            currentTypewriterProgress += DIALOG_MANAGER_TYPEWRITER_SPEED;
            int newProgress = (int)currentTypewriterProgress;

            if (newProgress > currentProgress)
            {
                currentTypewriterLine = {typewriterFont, currentTypewriterText.substr(0, newProgress)};
                currentTypewriterLine.setPos(10, 10);
            }
            currentTypewriterLine.step();
        }
    }
}

void DialogManager::draw() const
{
    jngl::setFontColor(0, 0, 0, 255);
    if (currentTypewriterProgress > 0)
    {
        currentTypewriterLine.draw();
    }

    if (isNarratorTextVisible)
    {
        jngl::setFontColor(255, 255, 255, 255);
        jngl::setColor(0, 0, 0, 178);
        jngl::drawRect(0, 1080 - BOX_HEIGHT, 1920, BOX_HEIGHT);
        currentNarratorText.draw();
    }

    size_t pos = BOX_HEIGHT * (choiceTexts.size() -1);
    int index = 0;
    for (auto& text : choiceTexts)
    {
        jngl::setFontColor(255, 255, 255, 255);
        jngl::setColor(0, 0, 0, 178);
        jngl::drawRect(-1920/2, 1080/2 - BOX_HEIGHT - pos, 1920, BOX_HEIGHT);
        pos -= BOX_HEIGHT;

        if (index == selected_index)
            jngl::setFontColor(255, 255, 0, 255);
        text.draw();
        index++;
    }

    if (bubble != nullptr)
    {
        jngl::setFontColor(0, 0, 0, 255);
        bubble->draw();
    }
}

bool DialogManager::isActive()
{
    return currentDialog != nullptr;
}

void DialogManager::showTypewriterAnimation(const std::string &text)
{
    currentTypewriterText = text;
    currentTypewriterProgress = 0;
}

void DialogManager::showNarratorText(const std::string &text)
{
    currentNarratorText.setText(text);
    currentNarratorText.setAlign(jngl::Alignment::CENTER);
    currentNarratorText.setCenter(960, 1040);
    isNarratorTextVisible = true;
}

void DialogManager::showChoices(std::shared_ptr<schnacker::AnswersStepResult> answers)
{
    choiceTexts.clear();
    size_t pos = BOX_HEIGHT * (answers->answers.size() -1);
    for (auto answerResult = answers->answers.begin(); answerResult != answers->answers.end(); ++answerResult)
    {
        schnacker::NodeId id;
        std::string text;
        std::tie(id, text) = (*answerResult);

        jngl::Text choiceText;
        choiceText.setFont(dialogFont);
        choiceText.setText(text);
        choiceText.setAlign(jngl::Alignment::LEFT);
        choiceText.setCenter(choiceText.getWidth() / 2.0 - 910, 1040/2 - pos);
        pos -= BOX_HEIGHT;
        choiceTexts.push_back(choiceText);
    }

    selected_index = 0;

    currentAnswers = answers;
}

void DialogManager::showCharacterText(std::string text, jngl::Vec2 pos)
{
    // TODO: use player pos in order to determine direction preference for bubble
    auto bubbleText = jngl::Text(text);
    bubbleText.setFont(dialogFont);
    if (auto _game = game.lock())
    {
        bubble = std::make_shared<SpeechBubble>(_game, "speechbubble",
                                                bubbleText, pos);
    }
}

void DialogManager::playCharacterVoice(std::string file)
{
    if (last_played_audio != "" && jngl::isPlaying(last_played_audio))
    {
        jngl::stop(last_played_audio);
    }

    try
    {
        jngl::play(file);
    }
    catch(const std::exception& e)
    {
        jngl::debugLn("Audiofile does not exist: " + file);
    }

    last_played_audio = file;
}

void DialogManager::playCharacterAnimation(std::string character, const std::string &id)
{
    if (auto _game = game.lock())
    {
        std::shared_ptr<SpineObject> spine_character = _game->getObjectById(character);
        if (spine_character)
        {
            auto animation = "say_" + _game->language + "_" + std::string(n_zero - std::min(n_zero, id.length()), '0') + id;
            spine_character->playAnimation(5, animation, false, (*_game->lua_state)["pass"]);
        }
    }
}

void DialogManager::continueCurrent()
{
    if (auto _game = game.lock())
    {
        if(currentDialog == nullptr || currentNode == nullptr)
        {
            dialog_callback();
            dialog_callback = (*_game->lua_state)["pass"];

            cancelDialog();
            return;
        }

        auto result = currentNode->step(currentDialog);
        currentNode = result->currentNode;

        auto textResult = std::dynamic_pointer_cast<schnacker::TextStepResult>(result);
        if(textResult)
        {
            std::string character = textResult->character->canonicalName;
            std::shared_ptr<SpineObject> spine_character = _game->getObjectById(character);
            if (spine_character)
            {
                auto optionalpos = spine_character->getPoint("speechbubble");
                if(optionalpos)
                    bubble_pos = spine_character->getPosition() + optionalpos.value();
            }
            showCharacterText(textResult->text, bubble_pos);
            std::string fileName = textResult->nodeId;
            auto fullFileName = "audio/" + _game->language + "_" + std::string(n_zero - std::min(n_zero, fileName.length()), '0') + fileName  + ".ogg";
            playCharacterAnimation(character, fileName);
            try {
                playCharacterVoice(fullFileName);
            }catch (const std::runtime_error& e) {
                jngl::debugLn("\033[1;31m Failed to load: " + fullFileName + "\033[0m");
            }
        }
        else
        {
            auto answersResult = std::dynamic_pointer_cast<schnacker::AnswersStepResult>(result);

            if(answersResult)
                showChoices(answersResult);
        }
    }
}

void DialogManager::cancelDialog()
{
    hideChoices();
    hideCharacterText();
    currentDialog = nullptr;
    currentNode = nullptr;
    currentAnswers = nullptr;
}

void DialogManager::selectCurrentAnswer()
{
    if(currentAnswers == nullptr
        || currentDialog == nullptr ||
        selected_index < 0)
    {
        return;
    }

    currentNode = currentAnswers->chooseAnswer(currentDialog, std::get<0>(currentAnswers->answers[selected_index]));
    selected_index = -1;
    hideChoices();
    continueCurrent();
}

void DialogManager::hideChoices()
{
    choiceTexts.clear();
    currentAnswers = nullptr;
}

void DialogManager::hideCharacterText()
{
    bubble = nullptr;
}

bool DialogManager::isSelectTextActive()
{
    return choiceTexts.size() != 0;
}

bool DialogManager::isOverText(jngl::Vec2 mouse_pos)
{
    return mouse_pos.y > 1040 - BOX_HEIGHT * choiceTexts.size();
}

void DialogManager::setSpeechBubblePosition(jngl::Vec2 position)
{
    bubble_pos = position;
    if (bubble)
        bubble->setPosition(position);
}
