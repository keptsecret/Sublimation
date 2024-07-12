#pragma once

namespace sublimation {

class Engine {
protected:
    Engine() = default;

public:
    ~Engine() = default;
    static Engine* getSingleton();

    bool isValidationLayersEnabled() const { return useValidationLayers; }
    Scene activeScene;

private:
    bool useValidationLayers = true;
};

} // namespace sublimation
