#include "QuadTree.h"

//** Rect **//
Rect::Rect(const Rect &other) : Rect(other.x, other.y, other.width, other.height) { }
Rect::Rect(double _x, double _y, double _width, double _height) :
    x(_x),
    y(_y),
    width(_width),
    height(_height) {
}
bool Rect::contains(const Rect &other) const noexcept {
    if (x > other.x) return false;
    if (y > other.y) return false;
    if (x + width  < other.x + other.width) return false;
    if (y + height < other.y + other.height) return false;
    return true; // within bounds
}
bool Rect::intersects(const Rect &other) const noexcept {
    if (x > other.x + other.width)  return false;
    if (x + width < other.x)        return false;
    if (y > other.y + other.height) return false;
    if (y + height < other.y)       return false;
    return true; // intersection
}
double Rect::getLeft()   const noexcept { return x - (width  * 0.5f); }
double Rect::getTop()    const noexcept { return y + (height * 0.5f); }
double Rect::getRight()  const noexcept { return x + (width  * 0.5f); }
double Rect::getBottom() const noexcept { return y - (height * 0.5f); }

//** Collidable **//
Collidable::Collidable(const Rect &_bounds, std::any _data) :
    bound(_bounds),
    data(_data) {
};

//** QuadTree **//
QuadTree::QuadTree() : QuadTree({}, 0, 0) { }
QuadTree::QuadTree(const QuadTree &other) : QuadTree(other.bounds, other.capacity, other.maxLevel) { }
QuadTree::QuadTree(const Rect &_bound, unsigned _capacity, unsigned _maxLevel) :
    bounds(_bound),
    capacity(_capacity),
    maxLevel(_maxLevel) {
    objects.reserve(_capacity);
    foundObjects.reserve(_capacity);
}

// Inserts an object into this quadtree
bool QuadTree::insert(Collidable *obj) {
    if (obj->qt != nullptr) return false;

    if (!isLeaf) {
        // insert object into leaf
        if (QuadTree *child = getChild(obj->bound))
            return child->insert(obj);
    }
    objects.push_back(obj);
    obj->qt = this;

    // Subdivide if required
    if (isLeaf && level < maxLevel && objects.size() >= capacity) {
        subdivide();
        update(obj);
    }
    return true;
}

// Removes an object from this quadtree
bool QuadTree::remove(Collidable *obj) {
    if (obj->qt == nullptr) return false; // Cannot exist in vector
    if (obj->qt != this) return obj->qt->remove(obj);

    objects.erase(std::find(objects.begin(), objects.end(), obj));
    obj->qt = nullptr;
    discardEmptyBuckets();
    return true;
}

// Removes and re-inserts object into quadtree (for objects that move)
bool QuadTree::update(Collidable *obj) {
    if (!remove(obj)) return false;

    // Not contained in this node -- insert into parent
    if (parent != nullptr && !bounds.contains(obj->bound))
        return parent->insert(obj);
    if (!isLeaf) {
        // Still within current node -- insert into leaf
        if (QuadTree *child = getChild(obj->bound))
            return child->insert(obj);
    }
    return insert(obj);
}

// Searches quadtree for objects within the provided boundary and returns them in vector
std::vector<Collidable*> &QuadTree::getObjectsInBound(const Rect &bound) {
    foundObjects.clear();
    for (const auto &obj : objects) {
        // Only check for intersection with OTHER boundaries
        if (&obj->bound != &bound && obj->bound.intersects(bound))
            foundObjects.push_back(obj);
    }
    if (!isLeaf) {
        // Get objects from leaves
        if (QuadTree *child = getChild(bound)) {
            child->getObjectsInBound(bound);
            foundObjects.insert(foundObjects.end(), child->foundObjects.begin(), child->foundObjects.end());
        } else for (QuadTree *leaf : children) {
            if (leaf->bounds.intersects(bound)) {
                leaf->getObjectsInBound(bound);
                foundObjects.insert(foundObjects.end(), leaf->foundObjects.begin(), leaf->foundObjects.end());
            }
        }
    }
    return foundObjects;
}

// Returns total children count for this quadtree
unsigned QuadTree::totalChildren() const noexcept {
    unsigned total = 0;
    if (isLeaf) return total;
    for (QuadTree *child : children)
        total += child->totalChildren();
    return 4 + total;
}

// Returns total object count for this quadtree
unsigned QuadTree::totalObjects() const noexcept {
    unsigned total = (unsigned)objects.size();
    if (!isLeaf) {
        for (QuadTree *child : children)
            total += child->totalObjects();
    }
    return total;
}

// Removes all objects and children from this quadtree
void QuadTree::clear() noexcept {
    if (!objects.empty()) {
        for (auto&& obj : objects)
            obj->qt = nullptr;
        objects.clear();
    }
    if (!isLeaf) {
        for (QuadTree *child : children)
            child->clear();
        isLeaf = true;
    }
}

// Subdivides into four quadrants
void QuadTree::subdivide() {
    double width = bounds.width  * 0.5f;
    double height = bounds.height * 0.5f;
    double x = 0, y = 0;
    for (unsigned i = 0; i < 4; ++i) {
        switch (i) {
            case 0: x = bounds.x + width; y = bounds.y; break; // Top right
            case 1: x = bounds.x;         y = bounds.y; break; // Top left
            case 2: x = bounds.x;         y = bounds.y + height; break; // Bottom left
            case 3: x = bounds.x + width; y = bounds.y + height; break; // Bottom right
        }
        children[i] = new QuadTree({ x, y, width, height }, capacity, maxLevel);
        children[i]->level  = level + 1;
        children[i]->parent = this;
    }
    isLeaf = false;
}

// Discards buckets if all children are leaves and contain no objects
void QuadTree::discardEmptyBuckets() {
    if (!objects.empty()) return;
    if (!isLeaf) {
        for (QuadTree *child : children)
            if (!child->isLeaf || !child->objects.empty())
                return;
    }
    if (clear(), parent != nullptr)
        parent->discardEmptyBuckets();
}

// Returns child that contains the provided boundary
QuadTree *QuadTree::getChild(const Rect &bound) const noexcept {
    bool left  = bound.x + bound.width < bounds.getRight();
    bool right = bound.x > bounds.getRight();

    if (bound.y + bound.height < bounds.getTop()) {
        if (left)  return children[1]; // Top left
        if (right) return children[0]; // Top right
    } else if (bound.y > bounds.getTop()) {
        if (left)  return children[2]; // Bottom left
        if (right) return children[3]; // Bottom right
    }
    return nullptr; // Cannot contain boundary -- too large
}

QuadTree::~QuadTree() {
    clear();
    if (children[0]) delete children[0];
    if (children[1]) delete children[1];
    if (children[2]) delete children[2];
    if (children[3]) delete children[3];
}
