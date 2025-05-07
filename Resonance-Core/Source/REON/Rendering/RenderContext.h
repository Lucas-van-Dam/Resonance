#pragma once

namespace REON {
	class RenderContext
	{
	public:
		virtual void init() = 0;
		virtual void swapBuffers() = 0;
		virtual void cleanup() = 0;
		virtual void resize() = 0;
	};
}
