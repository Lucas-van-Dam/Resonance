#pragma once

namespace REON {
	class RenderContext
	{
	public:
		virtual void init() = 0;
		virtual void startFrame() = 0;
		virtual void endFrame() = 0;
		virtual void cleanup() = 0;
		virtual void resize() = 0;
	};
}
