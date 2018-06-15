#include <time.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include "QuadTree_SFML_DEMO.h"

Rect MAP_BOUNDS = { 0, 0, 1280, 720 };

struct Object {
    Object(double _x, double _y, double _width, double _height) {
        item = Collidable({ _x, _y, _width, _height }, this);
        shape.setPosition((float)item.bound.x, (float)item.bound.y);
        shape.setSize(sf::Vector2f((float)item.bound.width, (float)item.bound.height));
    }
    void move() {
        if (item.bound.x + dx < 0 || item.bound.x + item.bound.width + dx > MAP_BOUNDS.width)
            dx = -dx;
        if (item.bound.y + dy < 0 || item.bound.y + item.bound.height + dy > MAP_BOUNDS.height)
            dy = -dy;
        item.bound.x += dx;
        item.bound.y += dy;
        shape.setPosition((float)item.bound.x, (float)item.bound.y);
    }
    double dx = (rand() % 201 - 100) * 0.05f;
    double dy = (rand() % 201 - 100) * 0.05f;
    sf::RectangleShape shape;
    Collidable item;
};

int main() {
    srand((unsigned)time(NULL));
    sf::RenderWindow window(sf::VideoMode((unsigned)MAP_BOUNDS.width, (unsigned)MAP_BOUNDS.height), "QuadTree");
    window.setFramerateLimit(60);
    window.setMouseCursorVisible(false);

    QuadTree map = QuadTree(MAP_BOUNDS, 8, 4);
    std::vector<Object*> objects;

    sf::Font font;
    font.loadFromFile("arial.ttf");
    map.setFont(font);

    sf::Text info("", font);
    info.setCharacterSize(20);
    info.setFillColor(sf::Color::Black);
    info.setPosition(sf::Vector2f(4, 4));

    sf::RectangleShape textBox;
    textBox.setFillColor(sf::Color(204, 204, 204));

    sf::Event event;
    sf::RectangleShape mouseBox;
    mouseBox.setOutlineThickness(3.0f);
    mouseBox.setFillColor(sf::Color(127, 0, 255, 0));
    mouseBox.setOutlineColor(sf::Color::Magenta);

    bool freezeObjects = false;
    Rect mouseBoundary = { 0, 0, 20, 20 };

    while (window.isOpen()) {
        // Update controls
        while (window.pollEvent(event) && event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
            // Esc = exit
            case sf::Keyboard::Escape:
                window.close();
                break;
            // F = freeze all objects
            case sf::Keyboard::F:
                freezeObjects = !freezeObjects;
                break;
            // C = clear quadtree and remove all objects
            case sf::Keyboard::C:
                map.clear();
                for (auto&& obj : objects)
                    delete obj;
                objects.clear();
                break;
            // Up = increase size of mouse box
            case sf::Keyboard::Up:
                mouseBoundary.width += 2;
                mouseBoundary.height += 2;
                break;
            // Down = decrease size of mouse box
            case sf::Keyboard::Down:
                mouseBoundary.width -= 2;
                mouseBoundary.height -= 2;
                break;
            }
        }
        window.clear();
        map.draw(window);

        // Update collisions
        std::vector<Object*> mouseCollisions;
        unsigned long long collisions = 0;
        unsigned long long qtCollisionChecks = 0;
        unsigned long long bfCollisionChecks = 0;
        for (auto&& obj : objects) {
            obj->shape.setFillColor(sf::Color::Blue);

            if (mouseBoundary.intersects(obj->item.bound)) {
                obj->shape.setFillColor(sf::Color::Red);
                mouseCollisions.push_back(obj);
                ++collisions;
            }
            for (auto&& otherObj : objects)
                ++bfCollisionChecks;
            for (auto&& found : map.getObjectsInBound_unchecked(obj->item.bound)) {
                ++qtCollisionChecks;
                if (&obj->item != found && found->bound.intersects(obj->item.bound)) {
                    ++collisions;
                    obj->shape.setFillColor(sf::Color::Red);
                }
            }
            if (!freezeObjects) {
                obj->move();
                map.update(&obj->item);
            }
            window.draw(obj->shape);
        }
        // Update mouse box
        mouseBoundary.x = sf::Mouse::getPosition(window).x;
        mouseBoundary.y = sf::Mouse::getPosition(window).y;
        mouseBox.setSize(sf::Vector2f((float)mouseBoundary.width, (float)mouseBoundary.height));
        mouseBox.setPosition((float)mouseBoundary.x, (float)mouseBoundary.y);

        // Left click = add objects
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && MAP_BOUNDS.contains(mouseBoundary)) {
            objects.push_back(new Object(mouseBoundary.getRight(), mouseBoundary.getTop(), rand() % 20 + 4, rand() % 20 + 4));
            map.insert(&objects.back()->item);
        }
        // Right click = remove objects within mouse box
        if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
            for (auto&& obj : mouseCollisions) {
                objects.erase(std::find(objects.begin(), objects.end(), obj));
                map.remove(&obj->item);
                delete obj;
            }
        }
        // Display quadtree debug info
        std::stringstream ss;
        ss << "Total Children: "                 << map.totalChildren()
           << "\nTotal Objects: "                << map.totalObjects()
           << "\nTotal Collisions: "             << collisions
           << "\nQuadTree collision checks: "    << qtCollisionChecks
           << "\nBrute force collision checks: " << bfCollisionChecks
           << "\nCollisions with mouse: "        << mouseCollisions.size()
           << "\nObjects in this quad: "         << map.getLeaf(mouseBoundary)->totalObjects();
        info.setString(ss.str());
        textBox.setSize(sf::Vector2f(info.getLocalBounds().width + 16, info.getLocalBounds().height + 16));
        window.draw(textBox);
        window.draw(info);
        window.draw(mouseBox);
        window.display();
    }
    // cleanup
    map.clear();
    for (auto&& obj : objects)
        delete obj;
    objects.clear();
}
