#pragma once

#include "spine_object.hpp"

#include <array>
#include <jngl.hpp>
#include <list>
#include <map>
#include <deque>

struct Node
{
    int G;
    int H;
    jngl::Vec2 coordinates;
    Node *parent;

    explicit Node(jngl::Vec2 coordinates_, Node *parent_ = nullptr);
    int getScore() const;
};

class Background : public SpineObject
{
public:
    explicit Background(const std::shared_ptr<Game> &game, const std::string &spine_file);
    ~Background() = default;

    bool is_walkable(jngl::Vec2 position) const;
    bool step(bool force = false) override;
    void draw() const override;

    std::deque<jngl::Vec2> getPathToTarget(jngl::Vec2 start, jngl::Vec2 target) const;

private:
    void stepSpineAndNavigation();

    std::vector<jngl::Vec2> corners;
    std::vector<std::vector<jngl::Vec2>> forbidden_corners;

    bool stepClickableRegions(bool force = false);
    std::vector<jngl::Vec2> getCorners() const;
    std::vector<std::vector<jngl::Vec2>> getForbiddenCorners() const;
    bool hasPathTo(jngl::Vec2 start, jngl::Vec2 target) const;
    static void releaseNodes(std::vector<Node *> &nodes_);
    static Node *findNodeOnList(const std::vector<Node *> &nodes_, jngl::Vec2 coordinates_);
    static int heuristic(jngl::Vec2 start, jngl::Vec2 target);
};
