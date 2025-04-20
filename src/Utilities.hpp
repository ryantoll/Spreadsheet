#ifndef RYANS_UTILITIES_HPP
#define RYANS_UTILITIES_HPP

#include "framework.hpp"

namespace RYANS_UTILITIES {

// Windows Utilities
#ifdef _WINDOWS

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
		SendMessage(window, EM_SETSEL, (WPARAM)(sel), (LPARAM)sel);
		SendMessage(window, EM_REPLACESEL, 0, (LPARAM)text.c_str());
	}

	// Append string to text of an Edit Box
	inline void Append_String_to_Edit_Box(HWND window, const std::string& text) { AppendWstringToEditBox(window, StringToWstring(text)); }
#endif // _WINDOWS

	// Tool used for parsing text.
	// Tests for enclosing chars and clears them out. Returns bool indicating success/failure.
	inline bool ClearEnclosingChars(const char c1, const char c2, std::string& text) {
		if (text[0] != c1 || text[text.size() - 1] != c2) { return false; }
		text.erase(0, 1);
		text.erase(text.size() - 1);
		return true;
	}

	// Tool used for parsing text.
	// Tests for enclosing wcahr_t and clears them out. Returns bool indicating success/failure.
	inline bool ClearEnclosingChars(const wchar_t c1, const wchar_t c2, std::wstring& text) {
		if (text[0] != c1 || text[text.size() - 1] != c2) { return false; }
		text.erase(0, 1);
		text.erase(text.size() - 1);
		return true;
	}
}

#endif // !__RYANS_UTILITIES_HPP
