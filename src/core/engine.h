#pragma once

namespace sublimation {

class Engine {
protected:
    Engine() {}

public:
    ~Engine() = default;
    static Engine* getSingleton();
};

} // namespace sublimation
