#include "stdafx.h"
#include "__WINDOW.h"

namespace WINDOWS_GUI {
	using namespace std;

	// Create window by passing parameters to WINDOW(HWND) constructor
	WINDOW ConstructChildWindow(const string& type, HWND parent, HMENU id, HINSTANCE inst) {
		auto h = CreateWindow(StringToWstring(type).c_str(), L"", WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, parent, id, inst, NULL);
		return WINDOW{ h };
	}

	WINDOW ConstructChildWindow(const string& type, HWND parent, long id, HINSTANCE inst) {
		return ConstructChildWindow(type, parent, reinterpret_cast<HMENU>(id), inst);
	}

	WINDOW ConstructTopLevelWindow(const string& type, HINSTANCE inst, const string& title) {
		auto h = CreateWindow(StringToWstring(type).c_str(), StringToWstring(title).c_str(), WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, nullptr, nullptr, inst, NULL);
		return WINDOW{ h };
	}

	// Control existing window
	WINDOW::WINDOW(HWND hWindow) : m_Handle(hWindow), 
		m_hParent(reinterpret_cast<HWND>(GetWindowLongPtr(m_Handle, GWLP_HWNDPARENT))),
		m_hInstance(reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_Handle, GWLP_HINSTANCE))),
		m_menu(reinterpret_cast<HMENU>(GetWindowLongPtr(m_Handle, GWLP_ID)))
	{ }
	

}
