# QuadTree-Cpp
Simple QuadTree written in C++ with SFML demo

![SFML Demo](https://i.imgur.com/whEL8hF.png)

#### Constructing a quadtree
```cpp
QuadTree *quadTree = new QuadTree({ x, y, width, height }, 8, 4);
```

#### Inserting objects
```cpp
// Inserts an object into the quadtree
int data = 5;
Collidable obj = Collidable({ x, y, width, height }, data);
quadTree->insert(&obj);
```

#### Removing objects
```cpp
// Removes an object from the quadtree
quadTree->remove(&obj);
```

#### Updating objects
```cpp
// Updates an object within the quadtree
// Useful for objects that move
obj.bound.x = 123.456;
obj.bound.y = 654.321;
quadTree->update(&obj);
```

#### Searching for objects within boundary
```cpp
// Getting objects within a particular boundary
Rect box = { x, y, width, height };
std::vector<Collidable*> foundObjects = quadTree->getObjectsInBound(box);
```

#### Getting objects & children count
```cpp
std::cout << quadTree->totalChildren() << "\n";
std::cout << quadTree->totalObjects() << "\n";
```

#### Clearing a quadtree
```cpp
quadTree->clear();
```

#### Getting data from a Collidable
```cpp
int stuff = 123;
Collidable obj = Collidable({ 0, 0, 10, 20}, stuff);

std::cout << std::any_cast<int>(obj.data) << '\n';
```
