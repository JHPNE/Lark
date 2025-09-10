// GlobalUndoRedo.h
#pragma once
#include "UndoRedo.h"

class GlobalUndoRedo
{
  public:
    static GlobalUndoRedo &Instance()
    {
        static GlobalUndoRedo instance;
        return instance;
    }

    UndoRedo &GetUndoRedo() { return m_undoRedo; }

  private:
    GlobalUndoRedo() = default;
    ~GlobalUndoRedo() = default;

    // Prevent copying
    GlobalUndoRedo(const GlobalUndoRedo &) = delete;
    GlobalUndoRedo &operator=(const GlobalUndoRedo &) = delete;

    UndoRedo m_undoRedo;
};
