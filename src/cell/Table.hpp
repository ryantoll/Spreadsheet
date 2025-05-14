#ifndef TABLE_CLASS_HPP
#define TABLE_CLASS_HPP

#include "Cell.hpp"
#include <memory>

#include <string>

class TABLE_BASE;
inline auto table = std::unique_ptr<TABLE_BASE>{ };

// TABLE_BASE is an abstract base class that is specialized for the OS in question.
// This decouples cell logic from OS dependence and maximizes portability.
// Implementations of this class act as an "Adapter" pattern to map platform specific GUI operations to a generic interface.
// Further abstractions may be considered in the future, such as cell windows, graphs, etc.
class TABLE_BASE {
public:
	virtual ~TABLE_BASE() { }

	virtual void AddRow() = 0;
	virtual void AddColumn() = 0;
	virtual void RemoveRow() = 0;
	virtual void RemoveColumn() = 0;
	virtual unsigned int GetNumColumns() const = 0;
	virtual unsigned int GetNumRows() const = 0;

	virtual void InitializeTable() = 0;
	virtual void Resize() = 0;
	virtual void Redraw() const = 0;

	virtual void FocusCell(const CELL::CELL_POSITION) const = 0;
	virtual void UnfocusCell(const CELL::CELL_POSITION) const = 0;
	virtual void FocusEntryBox() const = 0;
	virtual void UnfocusEntryBox(const CELL::CELL_POSITION) const = 0;

	virtual void FocusUp1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusDown1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusRight1(const CELL::CELL_POSITION) const = 0;
	virtual void FocusLeft1(const CELL::CELL_POSITION) const = 0;

	virtual CELL::CELL_PROXY CreateNewCell(const CELL::CELL_POSITION, const std::string&) const = 0;
	virtual void UpdateCell(const CELL::CELL_POSITION) const = 0;
	virtual void LockTargetCell(const CELL::CELL_POSITION) const = 0;
	virtual void ReleaseTargetCell() const = 0;
	virtual CELL::CELL_POSITION TargetCellGet() const = 0;

	virtual void Undo() const = 0;
	virtual void Redo() const = 0;
};

#endif //!TABLE_CLASS_HPP