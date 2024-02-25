#include <core/engine.h>

namespace sublimation {

Engine* Engine::getSingleton() {
    static Engine singleton;
    return &singleton;
}

} //namespace sublimation
