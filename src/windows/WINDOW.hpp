#ifndef WINDOWS_GUI_FOUNDATION_LIBRARY
#define WINDOWS_GUI_FOUNDATION_LIBRARY

#include <memory>
#include <string>
#include <Windows.h>

namespace RYANS_UTILITIES {
// Windows Utilities

// Wrapper for Windows char conversion utility function
inline std::wstring StringToWstring(const std::string& InputString) {
	size_t n = InputString.size() + 1;
	std::unique_ptr<wchar_t[]> OutputC_String(new wchar_t[n]);
	MultiByteToWideChar(CP_UTF8, 0, InputString.c_str(), -1, OutputC_String.get(), static_cast<int>(n));
	return OutputC_String.get();
}

// Wrapper for Windows char conversion utility function
inline std::string WstringToString(const std::wstring& InputWstring) {
	size_t n = InputWstring.size() + 1;
	std::unique_ptr<char[]> OutputC_String(new char[n]);
	WideCharToMultiByte(CP_UTF8, 0, InputWstring.c_str(), -1, OutputC_String.get(), static_cast<int>(n), NULL, NULL);
	return OutputC_String.get();
}

// Returns the wstring contained within an Edit Box
// May work for other window types with text or a title
inline std::wstring EditBoxToWstring(const HWND window) {
	int n = GetWindowTextLength(window) + 1;
	std::unique_ptr<wchar_t[]>OutputC_String(new wchar_t[n]);
	GetWindowText(window, OutputC_String.get(), n);
	return OutputC_String.get();
}

inline std::string EditBoxToString(const HWND window) {
	return WstringToString(EditBoxToWstring(window));
}

// Append wstring to text of an Edit Box
inline void AppendWstringToEditBox(HWND window, const std::wstring& text) {
	auto sel = GetWindowTextLength(window);
	SendMessage(window, EM_SETSEL, (WPARAM) (sel), (LPARAM) sel);
	SendMessage(window, EM_REPLACESEL, 0, (LPARAM) text.c_str());
}

// Append string to text of an Edit Box
inline void Append_String_to_Edit_Box(HWND window, const std::string& text) { AppendWstringToEditBox(window, StringToWstring(text)); }

namespace WINDOWS_GUI {

class WINDOW_POSITION {
	int m_xPos, m_yPos;
public:
	WINDOW_POSITION& X(const int x) { m_xPos = x; return *this; }
	WINDOW_POSITION& Y(const int y) { m_yPos = y; return *this; }
	[[nodiscard]] int X() const { return m_xPos; }
	[[nodiscard]] int Y() const { return m_yPos; }
};

class WINDOW_DIMENSIONS {
	int m_width, m_height;
public:
	WINDOW_DIMENSIONS& Width(const int w) { m_width = w; return *this; }
	WINDOW_DIMENSIONS& Height(const int h) { m_height = h; return *this; }
	[[nodiscard]] int Width() const { return m_width; }
	[[nodiscard]] int Height() const { return m_height; }
};

// This is a wrapper class to handle classic C-style Windows API calls in an object-oriented fashion
// A WINDOW is a non-owning reference to a specific window
// Any window thusly wrapped is still free to participate in C-style API calls as normal
// WINDOW implicitly converts to HWND for use in such functions
class WINDOW {
	HWND m_Handle{ nullptr };

public:
	WINDOW() = default;
	// Control existing window
	explicit WINDOW(HWND hWindow) : m_Handle(hWindow) { }

	HWND Handle() const { return m_Handle; }
	operator HWND() const { return m_Handle; }			// Allow for implicit conversion to HWND
	HWND Parent() const { return reinterpret_cast<HWND>(GetWindowLongPtr(m_Handle, GWLP_HWNDPARENT)); }
	HINSTANCE Instance() const { return reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_Handle, GWLP_HINSTANCE)); }
	HMENU ID() const { return Menu(); }
	HMENU Menu() const { return reinterpret_cast<HMENU>(GetWindowLongPtr(m_Handle, GWLP_ID)); }
	void Destroy() { DestroyWindow(m_Handle); }
	RECT GetClientRect() const { auto rekt = RECT{ }; ::GetClientRect(m_Handle, &rekt); return rekt; }
	WINDOW& Focus() { SetFocus(m_Handle); return *this; }
	const WINDOW& Focus() const { SetFocus(m_Handle); return *this; }
	WINDOW& Text(const std::string& title) { SetWindowText(m_Handle, StringToWstring(title).c_str()); return *this; }
	const WINDOW& Text(const std::string& title) const { SetWindowText(m_Handle, StringToWstring(title).c_str()); return *this; }
	WINDOW& Wtext(const std::wstring& title) { SetWindowText(m_Handle, title.c_str()); return *this; }
	const WINDOW& Wtext(const std::wstring& title) const { SetWindowText(m_Handle, title.c_str()); return *this; }
	WINDOW& Message(UINT message, WPARAM wparam, LPARAM lparam) { SendMessage(m_Handle, message, wparam, lparam); return *this; }
	const WINDOW& Message(UINT message, WPARAM wparam, LPARAM lparam) const { SendMessage(m_Handle, message, wparam, lparam); return *this; }
	WINDOW& Style(LONG_PTR style) { SetWindowLongPtr(m_Handle, GWL_STYLE, style); return *this; }
	const WINDOW& Style(LONG_PTR style) const { SetWindowLongPtr(m_Handle, GWL_STYLE, style); return *this; }

	[[nodiscard]] std::string Text() const { return EditBoxToString(m_Handle); }
	[[nodiscard]] std::wstring Wtext() const { return EditBoxToWstring(m_Handle); }
	[[nodiscard]] LONG_PTR Style() const { return GetWindowLongPtr(m_Handle, GWL_STYLE); }

	// Move, optionally resize
	WINDOW& Move(const WINDOW_POSITION position) {
		SetWindowPos(m_Handle, nullptr, position.X(), position.Y(), 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		return *this;
	}

	// Move, optionally resize
	const WINDOW& Move(const WINDOW_POSITION position) const {
		SetWindowPos(m_Handle, nullptr, position.X(), position.Y(), 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		return *this;
	}

	// Move, optionally resize
	WINDOW& Move(const WINDOW_POSITION position, const WINDOW_DIMENSIONS size) {
		SetWindowPos(m_Handle, nullptr, position.X(), position.Y(), size.Width(), size.Height(), SWP_NOZORDER);
		return *this;
	}

	// Move, optionally resize
	const WINDOW& Move(const WINDOW_POSITION position, const WINDOW_DIMENSIONS size) const {
		SetWindowPos(m_Handle, nullptr, position.X(), position.Y(), size.Width(), size.Height(), SWP_NOZORDER);
		return *this;
	}

	// Resize, optionally move
	WINDOW& Resize(const WINDOW_DIMENSIONS size) {
		SetWindowPos(m_Handle, nullptr, 0, 0, size.Width(), size.Height(), SWP_NOZORDER | SWP_NOMOVE);
		return *this;
	}

	// Resize, optionally move
	const WINDOW& Resize(const WINDOW_DIMENSIONS size) const {
		SetWindowPos(m_Handle, nullptr, 0, 0, size.Width(), size.Height(), SWP_NOZORDER | SWP_NOMOVE);
		return *this;
	}

	// Resize, optionally move
	WINDOW& Resize(const WINDOW_DIMENSIONS size, const WINDOW_POSITION position) {
		return Move(position, size);
	}

	// Resize, optionally move
	const WINDOW& Resize(const WINDOW_DIMENSIONS size, const WINDOW_POSITION position) const {
		return Move(position, size);
	}

	// Set the window procedure, returning the previous procedure
	// This can be used for sub-classing a window to add additional behavior
	WNDPROC Procedure(WNDPROC procedure) {
		return reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(procedure)));
	}
};

// Create a child window
inline WINDOW ConstructChildWindow(const std::string& type, HWND parent, HMENU id) {
	auto h = CreateWindow(StringToWstring(type).c_str(), L"", WS_CHILD | WS_BORDER | WS_VISIBLE,
		0, 0, 0, 0, parent, id, reinterpret_cast<HINSTANCE>(GetWindowLongPtr(parent, GWLP_HINSTANCE)), NULL);
	return WINDOW{ h };
}

// Create a child window
inline WINDOW ConstructChildWindow(const std::string& type, HWND parent, unsigned long id) {
	return ConstructChildWindow(type, parent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)));
}

// Create a window without a parent
inline WINDOW ConstructTopLevelWindow(const std::string& type, HINSTANCE inst, const std::string& title) {
	auto h = CreateWindow(StringToWstring(type).c_str(), StringToWstring(title).c_str(), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, inst, NULL);
	return WINDOW{ h };
}

} // namespace WINDOWS_GUI
} // namespace RYANS_UTILITIES

#endif // !WINDOWS_GUI_FOUNDATION_LIBRARY
