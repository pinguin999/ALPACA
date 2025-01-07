#pragma once

#include <jngl.hpp>

class MouseInfo {
public:
    void setMousePos(jngl::Vec2);

    struct Down {
        jngl::Vec2 newPos() const;
        bool released();

        Down(MouseInfo&, jngl::Vec2 objectPos);
        Down(const Down&) = delete;
        Down(Down&&) noexcept;
        Down& operator=(const Down&) = delete;
        Down& operator=(Down&&) noexcept;
        ~Down();

    private:
        MouseInfo* parent = nullptr;
        jngl::Vec2 startReference;
    };

    struct Over {
        jngl::Vec2 pos() const;

        /// calling this will result in all future calls of MouseInfo::pos returning nullopt
        ///
        /// objectPos will be used to calculate newPos
        std::optional<Down> pressed(jngl::Vec2 objectPos);

        explicit Over(MouseInfo&);

    private:
        MouseInfo& parent;
    };

    /// will return nullopt if the mouse is already over another object
    std::optional<Over> pos();

private:
    bool enabled = true;
    bool down = false;
    jngl::Vec2 mousePos;
};
