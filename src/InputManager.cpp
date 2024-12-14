#include "InputManager.h"

void InputManager::bindAction(int key, InputEventType eventType, const ActionCallback& callback) {
    actionBindings[key][eventType] = callback;
}

void InputManager::handleInput(GLFWwindow* window) {
    for (const auto& binding : actionBindings) {
        int key = binding.first;
        const auto& eventMap = binding.second;

        // Check if the key is pressed
        int state = glfwGetKey(window, key);

        if (state == GLFW_PRESS) {
            // If key wasn't already pressed, fire KeyPressed event
            if (pressedKeys.find(key) == pressedKeys.end() && eventMap.count(InputEventType::KeyPressed)) {
                eventMap.at(InputEventType::KeyPressed)();
            }
            pressedKeys.insert(key); // Mark as pressed
        } else if (state == GLFW_RELEASE) {
            // If key was pressed, fire KeyReleased event
            if (pressedKeys.find(key) != pressedKeys.end() && eventMap.count(InputEventType::KeyReleased)) {
                eventMap.at(InputEventType::KeyReleased)();
            }
            pressedKeys.erase(key); // Mark as released
        }
    }
}
