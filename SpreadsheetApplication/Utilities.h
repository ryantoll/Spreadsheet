#pragma once
#include "stdafx.h"
#include <string>
using std::string;
using std::wstring;
using std::unique_ptr;

inline wstring string_to_wstring(const string input_string) {

	size_t n = input_string.size() + 1;
	unique_ptr<wchar_t[]> output_C_string(new wchar_t[n]);		//Dynamically allocate new array to store conversion output.
	MultiByteToWideChar(CP_UTF8, 0, input_string.c_str(), -1, output_C_string.get(), n);		//Converts input string to wide string. Function requires C-style strings (i.e. null-terminated char arrays).

	return output_C_string.get();	//The.get() is required to return a built - in pointer as opposed to a library unique ptr.
	//unique_ptr automatically de-allocates and deletes the array it allocated when it goes out of scope.
}

inline string wstring_to_string(const wstring input_wstring) {

	size_t n = input_wstring.size() + 1;
	unique_ptr<char[]> output_C_string(new char[n]);		//Dynamically allocate new array to store conversion output.
	WideCharToMultiByte(CP_UTF8, 0, input_wstring.c_str(), -1, output_C_string.get(), n, NULL, NULL);	//Converts input wide string to string. Function requires C-style strings (i.e. null-terminated char arrays).

	return output_C_string.get();	//The.get() is required to return a built - in pointer as opposed to a library unique ptr.
	//unique_ptr automatically de-allocates and deletes the array it allocated when it goes out of scope.
}

inline wstring Edit_Box_to_Wstring(const HWND h) {

	size_t n = GetWindowTextLength(h) + 1;
	unique_ptr<wchar_t[]>output_C_string(new wchar_t[n]);
	GetWindowText(h, output_C_string.get(), n);

	return output_C_string.get();
}

// Tool used for parsing text.
// Tests for enclosing chars and clears them out. Returns bool indicating success/failure.
inline bool ClearEnclosingChars(const char c1, const char c2, string& s) {
	if (s[0] != c1 || s[s.size() - 1] != c2) { return false; }
	s.erase(0, 1);
	s.erase(s.size() - 1);
	return true;
}

inline bool ClearEnclosingChars(const wchar_t c1, const wchar_t c2, wstring& s) {
	if (s[0] != c1 || s[s.size() - 1] != c2) { return false; }
	s.erase(0, 1);
	s.erase(s.size() - 1);
	return true;
}