#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <GLFW/glfw3.h>

enum class InputEventType {
    KeyPressed,
    KeyReleased
};

// Define an action type
using ActionCallback = std::function<void()>;

// Input Manager Class
class InputManager {
public:
    // Bind a key to an action
    void bindAction(int key, InputEventType eventType, const ActionCallback& callback);

    // Bind a key to a state variable
    void bindKeyState(int key, bool* state);

    // Process input events
    void handleInput(GLFWwindow* window);

private:
    // Internal mapping of keys to callbacks
    std::unordered_map<int, std::unordered_map<InputEventType, ActionCallback>> actionBindings;

    // Map of key-to-boolean pointer for automatic state updates
    std::unordered_map<int, bool*> keyStates;

    // Set of currently pressed keys
    std::unordered_set<int> pressedKeys;
};

#endif // INPUTMANAGER_H
