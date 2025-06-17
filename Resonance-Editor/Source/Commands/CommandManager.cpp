#include "CommandManager.h"

namespace REON::EDITOR {
	std::unique_ptr<ICommand> CommandManager::m_CurrentBatch;
	std::vector<std::unique_ptr<ICommand>> CommandManager::m_UndoStack;
	std::vector<std::unique_ptr<ICommand>> CommandManager::m_RedoStack;

	void CommandManager::startBatch(std::unique_ptr<ICommand> command)
	{
		if(m_CurrentBatch) {
			REON_ERROR("A command batch is already in progress. Please end the current batch before starting a new one.");
			return;
		}
		m_CurrentBatch = std::move(command);
		REON_TRACE("Starting command batch");
	}

	void CommandManager::updateBatch(std::function<void(ICommand*)> updatefn)
	{
		if(m_CurrentBatch)
			updatefn(m_CurrentBatch.get());
	}

	void CommandManager::endBatch()
	{
		if (m_CurrentBatch) {
			m_CurrentBatch->execute();
			m_UndoStack.push_back(std::move(m_CurrentBatch));
			m_RedoStack.clear();
			m_CurrentBatch.reset();
			REON_TRACE("Ending command batch");
		}
	}

	void CommandManager::execute(std::unique_ptr<ICommand> command)
	{
		command->execute();
		if (!m_UndoStack.empty()) {
			if (m_UndoStack.back()->MergeWith(command.get())) {
				return;
			}
		}
		m_UndoStack.push_back(std::move(command));
		m_RedoStack.clear();
	}

	void CommandManager::undo()
	{
		if(m_UndoStack.empty())
			return;

		auto cmd = std::move(m_UndoStack.back());
		m_UndoStack.pop_back();
		cmd->undo();
		m_RedoStack.push_back(std::move(cmd));
	}

	void CommandManager::redo()
	{
		if(m_RedoStack.empty())
			return;

		auto cmd = std::move(m_RedoStack.back());
		m_RedoStack.pop_back();
		cmd->execute();
		m_UndoStack.push_back(std::move(cmd));
	}
}