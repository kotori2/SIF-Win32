// Adds support for touchscreen in Windows 7 and above.

#include <vector>
#include <stdint.h>

namespace Win32Touch
{
	enum TouchType
	{
		TouchDown,
		TouchUp,
		TouchMove,
		TouchUnknown = 255
	};
	struct TouchPoint
	{
		uint32_t TouchID;
		uint32_t X;
		uint32_t Y;
		TouchType Type;
	};

	typedef std::vector<TouchPoint> TouchInputList;

	// Always returns false in Windows Vista and below
	bool HasTouchCapabilities();

	// Returns list of touch
	std::vector<TouchPoint> GetTouchList(HWND Window, void* TouchInputHandle, int TouchCount);

	// Register window for touch capable. Does nothing in Windows Vista and below
	void RegisterWindowForTouch(HWND Window);

	// Free HTOUCHINPUT
	void ReleaseTouchHandle(void* TouchInputHandle);
}
