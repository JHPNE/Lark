#pragma once

#ifndef EDITOR_INTERFACE
#define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif // !EDITOR_INTERFACE

namespace engine {
    void cleanup_engine_systems();
}
