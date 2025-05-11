#ifndef RYANS_UTILITIES_HPP
#define RYANS_UTILITIES_HPP

#include <string>

namespace RYANS_UTILITIES {
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
