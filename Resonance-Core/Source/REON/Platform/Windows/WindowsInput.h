#pragma once

#include "REON/Input.h"

namespace REON {
	class WindowsInput : public Input
	{
	protected:
		bool IsKeyPressedImpl(int keyCode) const override;

		bool IsMouseButtonPressedImpl(int button) const override;
		std::pair<float, float> GetMousePositionImpl() const override;
		float GetMouseXImpl() const override;
		float GetMouseYImpl() const override;
	};
}

