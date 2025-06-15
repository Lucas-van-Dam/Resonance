#pragma once
#include <memory>
#include "ICommand.h"
#include <vector>
#include <functional>
#include <Reon.h>

namespace REON::EDITOR{

class CommandManager
{
public:
	static void startBatch(std::unique_ptr<ICommand>);
	static void updateBatch(std::function <void(ICommand*)>);
	static void endBatch();

	static void execute(std::unique_ptr<ICommand>);
	static void undo();
	static void redo();

	static bool canUndo() { return !m_UndoStack.empty(); }
	static bool canRedo() { return !m_RedoStack.empty(); }


private:
	static std::unique_ptr<ICommand> m_CurrentBatch;
	static std::vector<std::unique_ptr<ICommand>> m_UndoStack;
	static std::vector<std::unique_ptr<ICommand>> m_RedoStack;
};

}