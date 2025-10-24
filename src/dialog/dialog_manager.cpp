#include "dialog_manager.hpp"
#include "../game.hpp"

constexpr int BOX_HEIGHT = 65;
constexpr int BOX_PADDING = 20;
constexpr double CHOICE_BOX_TOP = 280;
constexpr int SPINE_MOUTH_TRACK = 5;


DialogManager::DialogManager(std::shared_ptr<Game> game)
    : dialogFont((*game->lua_state)["config"]["default_font"], 25),
      bubble(nullptr),
      selected_index(-1),
      dialog_callback((*game->lua_state)["pass"]),
      game(game),
      default_font_color(textToColor((*game->lua_state)["config"]["default_font_color"])),
      default_font_selected_color(textToColor((*game->lua_state)["config"]["default_font_selected_color"])),
      default_font_not_selected_color(textToColor((*game->lua_state)["config"]["default_font_not_selected_color"]))
{
}

void DialogManager::loadDialogsFromFile(const std::string& fileName, bool initializeVariables)
{
    if (auto _game = game.lock())
    {
        schnackFile = schnacker::SchnackFile::loadFromString(_game->lua_state, jngl::readAsset(fileName).str(), initializeVariables);
        schnackFile->setCurrentLocale(_game->language);
    }
}

void DialogManager::play(const std::string &dialogName, jngl::Vec2, const sol::function &callback)
{
    cancelDialog(); // if there is a current dialog, cancel the current one

    currentDialog = schnackFile->dialogs[dialogName];
    if (currentDialog)
    {
        currentNode = currentDialog->getEntryNode();
        if(currentNode != nullptr)
        {
            if(!currentNode->checkPrecondition(currentDialog))
            {
                currentNode = nullptr;
            }
        }
        dialog_callback = callback;
        continueCurrent();
    }else
    {
        jngl::error("Dialog " + dialogName + " not found.");
    }
}

void DialogManager::step()
{
    if(!currentDialog)
    {
        wasActiveLastFrame = false;
        return;
    }

    if (!last_played_audio.empty() && !jngl::isPlaying(last_played_audio) && choiceTexts.empty())
    {
        continueCurrent();
    }
    if (auto _game = game.lock())
    {
        if(!choiceTexts.empty())
        {
            auto pointer_position = _game->pointer->getPosition();

            size_t pos = CHOICE_BOX_TOP;
            int temp_index = 0;
            selected_index = -1;
            for (auto text = choiceTexts.begin(); text != choiceTexts.end(); text++)
            {
                if (pointer_position.y > pos - BOX_PADDING
                    && pointer_position.y < pos + BOX_HEIGHT - BOX_PADDING)
                {
                    selected_index = temp_index;
                    break;
                }
                temp_index++;
                pos += BOX_HEIGHT;
            }

            auto direction = _game->pointer->getMovementStep();
            if (boost::qvm::mag_sqr(direction - jngl::Vec2(0, 1)) < 0.5)
            {
                selected_index = std::max(selected_index + 1, 0);
            }

            if (boost::qvm::mag_sqr(direction - jngl::Vec2(0, -1)) < 0.5)
            {
                selected_index = std::min(selected_index - 1, (int)choiceTexts.size() - 1);
            }
        }

        // mouse click
        if(_game->pointer->primaryPressed() && !_game->pointer->isPrimaryAlreadyHandled())
        {
            if (!wasActiveLastFrame && isActive())
            {
                // if there is an option selected, take it
                if (selected_index != -1)
                {
                    selectCurrentAnswer(selected_index);
                    _game->pointer->setPrimaryHandled();
                }
                // if there are no options at all
                else if (choiceTexts.empty())
                {
                    stopCharacterVoiceAndAnimation();
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
    }
}

void DialogManager::draw() const
{
    if (bubble) {
        jngl::setFontColor(default_font_color);
        bubble->draw();
    }
    if (isSelectTextActive())
    {
        int index = 0;
        for (auto& text : choiceTexts)
        {

            jngl::setFontColor(default_font_not_selected_color);

            if (index == selected_index)
                jngl::setFontColor(default_font_selected_color);
            text.draw();
            index++;
        }
    }
}

bool DialogManager::isActive()
{
    return currentDialog != nullptr;
}

void DialogManager::showChoices(std::shared_ptr<schnacker::AnswersStepResult> answers)
{
    choiceTexts.clear();
    size_t pos = CHOICE_BOX_TOP;// BOX_HEIGHT * (answers->answers.size() -1);
    for (auto answerResult = answers->answers.begin(); answerResult != answers->answers.end(); ++answerResult)
    {
        schnacker::NodeId id;
        std::string text;
        std::tie(id, text) = (*answerResult);

        jngl::Text choiceText;
        choiceText.setFont(dialogFont);
        choiceText.setText(text);
        choiceText.setAlign(jngl::Alignment::LEFT);
        choiceText.setY(pos);
        choiceText.setX(-900);
        pos += BOX_HEIGHT;
        choiceTexts.push_back(choiceText);
    }

    if (auto _game = game.lock())
    {
        bubble = std::make_shared<SpeechBubble>(_game, "speechbubble", jngl::Text(), jngl::Text(), 0xffffffff_rgba);
    }

    selected_index = 0;

    currentAnswers = answers;
}

jngl::Rgba DialogManager::textToColor(const std::string& color_text)
{
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int a = 255;

    std::stringstream ssr;
    ssr << std::hex << color_text.substr(1, 2);
    ssr >> r;

    std::stringstream ssg;
    ssg << std::hex << color_text.substr(3, 2);
    ssg >> g;

    std::stringstream ssb;
    ssb << std::hex << color_text.substr(5, 2);
    ssb >> b;

    if (color_text.size() == 9)
    {
        std::stringstream ssa;
        ssa << std::hex << color_text.substr(7, 2);
        ssa >> a;
    }

    return jngl::Rgba::u8(r, g, b, a);
}


void DialogManager::showCharacterText(std::shared_ptr<schnacker::TextStepResult> text)
{
    // TODO: use player pos in order to determine direction preference for bubble
    auto bubbleText = jngl::Text(text->text);
    bubbleText.setFont(dialogFont);

    auto characterName = jngl::Text(text->character->displayName);
    characterName.setFont(dialogFont);

    jngl::Rgba textColor = 0xffffffff_rgba;

    if (text->character->color.size() == 7)
    {
        textColor = textToColor(text->character->color);
    }

    if (auto _game = game.lock())
    {
        bubble = std::make_shared<SpeechBubble>(_game, "speechbubble",
                                                bubbleText, characterName, textColor);
    }
}

void DialogManager::playCharacterVoice(const std::string &file)
{
    if (!last_played_audio.empty() && jngl::isPlaying(last_played_audio))
    {
        jngl::stop(last_played_audio);
        last_played_audio = "";
    }

    try
    {
        jngl::play(file);
        last_played_audio = file;
    }
    catch(std::exception&)
    {
        jngl::error("Audiofile does not exist: " + file);
        last_played_audio = "";
    }

}

void DialogManager::stopCharacterVoiceAndAnimation() {
	if (!last_played_audio.empty() && jngl::isPlaying(last_played_audio)) {
		jngl::stop(last_played_audio);
	}
	if (auto _game = game.lock()) {
		std::shared_ptr<SpineObject> spine_character = _game->getObjectById(last_played_audio_character);
		if (spine_character) {
			spine_character->stopAnimation(SPINE_MOUTH_TRACK);
            last_played_audio_character = "";
		}
	}
}

void DialogManager::playCharacterAnimation(const std::string &character, const std::string &id)
{
    if (auto _game = game.lock())
    {
        std::shared_ptr<SpineObject> spine_character = _game->getObjectById(character);
        if (spine_character)
        {
            auto animation = "say_" + _game->language + "_" + std::string(n_zero - std::min(n_zero, id.length()), '0') + id;
            spine_character->playAnimation(SPINE_MOUTH_TRACK, animation, false, (*_game->lua_state)["pass"]);
            last_played_audio_character = character;
        }
    }
}

void DialogManager::continueCurrent()
{
    if (auto _game = game.lock())
    {
        if(currentDialog == nullptr || currentNode == nullptr)
        {
            cancelDialog();
            return;
        }

        auto result = currentNode->step(currentDialog);
        if(result == nullptr){
            cancelDialog();
            return;
        }
        currentNode = result->currentNode;
        currentDialog = result->currentDialog;

        auto textResult = std::dynamic_pointer_cast<schnacker::TextStepResult>(result);
        if(textResult)
        {
            std::string character = textResult->character->canonicalName;
            showCharacterText(textResult);
            std::string fileName = textResult->nodeId;
            auto fullFileName = "audio/" + _game->language + "_" + std::string(n_zero - std::min(n_zero, fileName.length()), '0') + fileName  + ".ogg";
            playCharacterAnimation(character, fileName);
            try {
                playCharacterVoice(fullFileName);
            }catch (const std::runtime_error& e) {
                jngl::error("\033[1;31m Failed to load: " + fullFileName + "\033[0m");
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

    if (auto _game = game.lock())
    {
        dialog_callback();
        dialog_callback = (*_game->lua_state)["pass"];
    }
}

void DialogManager::selectCurrentAnswer(int index)
{
    if(currentAnswers == nullptr
        || currentDialog == nullptr ||
        index < 0)
    {
        return;
    }

    currentNode = currentAnswers->chooseAnswer(currentDialog, std::get<0>(currentAnswers->answers[index]));
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

bool DialogManager::isSelectTextActive() const
{
    return !choiceTexts.empty();
}

bool DialogManager::isOverText(jngl::Vec2 mouse_pos)
{
    return mouse_pos.y > 1040 - BOX_HEIGHT * choiceTexts.size();
}
