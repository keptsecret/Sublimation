#pragma once

namespace sublimation {

class Engine {
protected:
    Engine() {}

public:
    ~Engine() = default;
    static Engine* getSingleton();

    bool isValidationLayersEnabled() const { return useValidationLayers; }

private:
    bool useValidationLayers = true;
};

} // namespace sublimation
