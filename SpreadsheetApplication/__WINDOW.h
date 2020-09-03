#ifndef WINDOWS_GUI_FOUNDATION_LIBRARY
#define WINDOWS_GUI_FOUNDATION_LIBRARY

#include "stdafx.h"
//#include <Windows.h>
//#include <string>
#include "Utilities.h"

namespace WINDOWS_GUI {
	using namespace RYANS_UTILITIES;

	class WINDOW_POSITION {
		int m_xPos, m_yPos;
	public:
		WINDOW_POSITION& X(const int x) noexcept { m_xPos = x; return *this; }
		WINDOW_POSITION& Y(const int y) noexcept { m_yPos = y; return *this; }
		int X() const noexcept { return m_xPos; }
		int Y() const noexcept { return m_yPos; }
	};

	class WINDOW_DIMENSIONS {
		int m_width, m_height;
	public:
		WINDOW_DIMENSIONS& Width(const int w) noexcept { m_width = w; return *this; }
		WINDOW_DIMENSIONS& Height(const int h) noexcept { m_height = h; return *this; }
		int Width() const noexcept { return m_width; }
		int Height() const noexcept { return m_height; }
	};

	// This is a wrapper class to handle classic C-style Windows API calls in an object-oriented fashion
	// A WINDOW is a non-owning reference to a specific window
	// Any window thusly wrapped is still free to participate in C-style API calls as normal
	// WINDOW implicitly converts to HWND for use in such functions
	class WINDOW {
		HWND m_Handle{ nullptr};
		HWND m_hParent{ nullptr};
		HINSTANCE m_hInstance{ nullptr };
		HMENU m_menu{ nullptr };
		std::string m_WindowType;

	public:
		WINDOW() = default;
		explicit WINDOW(HWND hWindow);
		WINDOW(const WINDOW&) = default;
		WINDOW(WINDOW&&) = default;
		WINDOW& operator=(const WINDOW&) = default;
		WINDOW& operator=(WINDOW&&) = default;
		virtual ~WINDOW() { }

		HWND Handle() const noexcept { return m_Handle; }
		operator HWND() const noexcept { return m_Handle; }			// Allow for implicit conversion to HWND
		HWND Parent() const noexcept { return m_hParent; }
		HMENU ID() const noexcept { return m_menu; }
		HMENU Menu() const noexcept { return m_menu; }
		void Destroy() noexcept { DestroyWindow(m_Handle); }
		RECT GetClientRect() const noexcept { auto rekt = RECT{ }; ::GetClientRect(m_Handle, &rekt); return rekt; }
		WINDOW& Focus() noexcept { SetFocus(m_Handle); return *this; }
		const WINDOW& Focus() const noexcept { SetFocus(m_Handle); return *this; }
		WINDOW& Text(const std::string& title) noexcept { SetWindowText(m_Handle, string_to_wstring(title).c_str()); return *this; }
		const WINDOW& Text(const std::string& title) const noexcept { SetWindowText(m_Handle, string_to_wstring(title).c_str()); return *this; }
		WINDOW& Wtext(const std::wstring& title) noexcept { SetWindowText(m_Handle, title.c_str()); return *this; }
		const WINDOW& Wtext(const std::wstring& title) const noexcept { SetWindowText(m_Handle, title.c_str()); return *this; }
		WINDOW& Message(UINT message, WPARAM wparam, LPARAM lparam) noexcept { SendMessage(m_Handle, message, wparam, lparam); return *this; }
		const WINDOW& Message(UINT message, WPARAM wparam, LPARAM lparam) const noexcept { SendMessage(m_Handle, message, wparam, lparam); return *this; }

		[[nodiscard]] std::string Text() const noexcept { return Edit_Box_to_String(m_Handle); }
		[[nodiscard]] std::wstring Wtext() const noexcept { return Edit_Box_to_Wstring(m_Handle); }

		// Move, optionally resize, optionally repaint
		// Specifying repaint == true requires that size be given as well, even if it is default-constructed
		WINDOW& Move(const WINDOW_POSITION position, const WINDOW_DIMENSIONS size = WINDOW_DIMENSIONS{ }, const bool repaint = true) noexcept {
			MoveWindow(m_Handle, position.X(), position.Y(), size.Width(), size.Height(), repaint);
			return *this;
		}

		// Move, optionally resize, optionally repaint
		// Specifying repaint == true requires that size be given as well, even if it is default-constructed
		WINDOW& Resize(const WINDOW_DIMENSIONS size, const WINDOW_POSITION position = WINDOW_POSITION{ }, const bool repaint = true) noexcept {
			return Move(position, size, repaint);
		}
		
		// Set the window procedure, returning the previous procedure
		// This can be used for sub-classing a window to add additional behavior
		WNDPROC Procedure(WNDPROC procedure) noexcept { 
			return reinterpret_cast<WNDPROC>(SetWindowLongPtr(m_Handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(procedure)));
		}
	};

	WINDOW ConstructChildWindow(const std::string& type, HWND parent, HMENU id, HINSTANCE inst);
	WINDOW ConstructChildWindow(const std::string& type, HWND parent, long id, HINSTANCE inst);
	WINDOW ConstructTopLevelWindow(const std::string& type, HINSTANCE inst, const std::string& title);
}

#endif // !WINDOWS_GUI_FOUNDATION_LIBRARY
