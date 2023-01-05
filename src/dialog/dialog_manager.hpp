#pragma once

#include <string>
#include <list>
#include <jngl.hpp>
#include <jngl/Vec2.hpp>
#include <sol/sol.hpp>
#include <schnacker.hpp>

#include "speech_bubble.hpp"

class Game;

#define DIALOG_MANAGER_TYPEWRITER_SPEED 0.3f

class DialogManager
{
public:
    explicit DialogManager(std::shared_ptr<Game> game);
    void loadDialogsFromFile(std::string fileName);

    void step();
    void draw() const;

    bool isActive();
    void cancelDialog();

    bool isSelectTextActive();
    bool isOverText(jngl::Vec2 mouse_pos);

    void play(std::string characterName, jngl::Vec2 pos, sol::function callback); // TODO: multiple positions for different characters
    void continueCurrent();
    void selectCurrentAnswer();

    void setSpeechBubblePosition(jngl::Vec2 position);

private:
    void showTypewriterAnimation(const std::string &text);
    void showNarratorText(const std::string &text);
    void showChoices(std::shared_ptr<schnacker::AnswersStepResult> answers);
    void showCharacterText(std::string text, jngl::Vec2 pos);
    void playCharacterVoice(std::string file);
    void playCharacterAnimation(std::string id);
    void hideChoices();
    void hideCharacterText();

    std::shared_ptr<schnacker::AnswersStepResult> currentAnswers = nullptr;

    std::shared_ptr<schnacker::SchnackFile> schnackFile = nullptr;
    std::shared_ptr<schnacker::Dialog> currentDialog = nullptr;
    std::shared_ptr<schnacker::Node> currentNode = nullptr;

    jngl::Font typewriterFont;
    jngl::Font dialogFont;
    std::string currentTypewriterText;
    float currentTypewriterProgress = -1;
    jngl::TextLine currentTypewriterLine;
    jngl::Text currentNarratorText;
    std::list<jngl::Text> choiceTexts;
    bool isNarratorTextVisible;
    std::shared_ptr<SpeechBubble> bubble;
    jngl::Vec2 bubble_pos = {0, 0};
    int selected_index;
    std::string last_played_audio = "";
    bool wasActiveLastFrame = false;
    sol::function dialog_callback;
	const std::weak_ptr<Game> game;

    size_t n_zero = 3;
};
