#pragma once

#include <string>
#include <list>
#include <jngl.hpp>
#include <jngl/Vec2.hpp>
#include <sol/sol.hpp>
#include <schnacker.hpp>

#include "speech_bubble.hpp"

class Game;

class DialogManager
{
public:
    explicit DialogManager(std::shared_ptr<Game> game);
    void loadDialogsFromFile(std::string fileName, bool initializeVariables);

    void step();
    void draw() const;

    bool isActive();
    void cancelDialog();

    bool isSelectTextActive() const;
    bool isOverText(jngl::Vec2 mouse_pos);

    void play(const std::string &characterName, jngl::Vec2 pos, const sol::function &callback); // TODO: multiple positions for different characters
    void continueCurrent();
    void selectCurrentAnswer(int selected_index);
    jngl::Rgba textToColor(std::string color_text);

#ifndef NDEBUG
    int getChoiceTextsSize(){return int(choiceTexts.size());};
#endif
private:
    void showChoices(std::shared_ptr<schnacker::AnswersStepResult> answers);
    void showCharacterText(std::shared_ptr<schnacker::TextStepResult> text);
    void playCharacterVoice(const std::string &file);
    void playCharacterAnimation(const std::string &character, const std::string &id);
    void hideChoices();
    void hideCharacterText();

    std::shared_ptr<schnacker::AnswersStepResult> currentAnswers;

    std::shared_ptr<schnacker::SchnackFile> schnackFile;
    std::shared_ptr<schnacker::Dialog> currentDialog;
    std::shared_ptr<schnacker::Node> currentNode;

    jngl::Font dialogFont;
    std::list<jngl::Text> choiceTexts;
    std::shared_ptr<SpeechBubble> bubble;
    int selected_index;
    std::string last_played_audio;
    bool wasActiveLastFrame = false;
    sol::function dialog_callback;
	const std::weak_ptr<Game> game;

    jngl::Rgba default_font_color;
    jngl::Rgba default_font_selected_color;
    jngl::Rgba default_font_not_selected_color;

    size_t n_zero = 3;
};
