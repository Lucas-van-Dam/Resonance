#pragma once

namespace REON::EDITOR {
	class ICommand {
	public:
		virtual ~ICommand() = default;
		virtual bool MergeWith(const ICommand* other) { return false; };
		virtual void execute() = 0;
		virtual void undo() = 0;
	};
}