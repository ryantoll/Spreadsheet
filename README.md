This project is meant to demonstrate one specific focus of study: the Gang of Four design principles. It is intended to be a spreadsheet application similar to Microsoft Excel. It is somewhat functional in it's current state. The spreadsheet can currently handle text, numbers, and active references properly. In progress are functions, which only offers a few functions and can only handle limited complexity. That said, the basic structure is in place and functions can be nested recursively. (i.e. =AVERAGE(1,&R1C1,3,SUM(4),5)) However, parsing will presently encounter an error if nested functions have multiple values since it picks up the internal comma (which is on the top of the queue to fix). Further, parsing has not yet been made to handle operators (+,-,*,/), nor can it handle () for order of operations. I may need to rethink the parsing scheme to use an object-oriented rather than procedural approach to make the various complexities easier to handle.

The core functionality is held within the Cell and Table classes. There is a little bit of interest in the implementation of SpreadsheetApplication.cpp, but it mostly just bridges Windows OS events/commands to Table public member functions. The other files are fairly standard resources and infrastructure for a Windows application.

The Table header outlines an abstract base class TABLE to represent the GUI. This model decouples the GUI implementation from the lower-level data management. WINDOWS_TABLE inherets from TABLE to provide an implementation specific to a Windows environment. It also defines a helper class CELL_ID, which aids in mapping a GUI cell to the appropraite cell data. Only a base-level Windows implementation is provided. This includes TABLE operations as well as additional functionality for the cell windows. Other implementaitons, for Windows or other OS's, could easily be added since proper decoupling is utilized.

CELL_ID demonstrates the "Builder" pattern to allow for clear, fluent usage. One problem this solves is mixing up constructor arguments. Rows & columns could easily be flipped and it may be hard to back track such errors. By using a builder pattern, the client programmer must clearly state each assignment as either a row or column. Mistakes will be minimized and may stand out more clearly with such clear syntax. A fluent model is also used so that the programmer may smoothly chain together member function calls that are conceptually related. (I.e. CELL_ID.SetRow().SetColumn();)

The Cell header defines a common base class for all cells as well as implementing a "Factory" pattern for cell creation. Each type of cell inherets from CELL and adds additional functionality as needed for its implementaiton. The factory function creates the appropriate cell based off of user input and manages the data structure that holds cell data. This function also produces notifications so that cells know when data they reference is changed. Each cell edit deletes the old cell and creates it anew to change cell types and push updates as needed. As of this writing, only text, number, and reference cells have been fully completed. Function cells have been outlined, but only work for trivial cases such as single values or references.

A reference is made starting with the '&' character followed by "R___C___" filling in the appropriate row & column numbers. (Case and order do not matter. Adding other characters beyond this will produce invalid input and throw an error.) Reference cells update as the referenced cell is changed by utilizing an "Observer" pattern. The cell factory function subscribes and unsubscribes reference cells as they are created and destroyed. Dangling references throw an error: "!REF!"

In Progress: Function Cells
Function cells are created by starting with '='. A function cell is a cell that contains a single function, a vector of arguments, and a single result for display. Each argument may be either a single value, a reference to another cell, or another function. As such, functions can be recursively composed to contain any number of sub-functions. Function cells make use of the "Strategy" pattern to allow for a multiplicity of functionallities utilizing a consistent interface. (Each FUCNTION object has an operation it performs on its vector of ARGUMENTS to produce a single result, which itself is an ARGUMENT to enable recursion.) This option of specializing a generic base function object for each operation is selected to fit the goal of demonstrating a variety of OOP design patterns. One possible alternative is to store a function pointer that can be assigned the appropraite function (such as a lambda or other callable object) as opposed to creating a subclass for each possible function the program recognizes.

The ARGUMENT object used by FUNCTIONs makes use of the "Composite" design pattern. This allows a FUNCTION to treat all arguments as a single value, ignoring any underlying complexity. A FUNCTION simply calls .get() on the stored future to interpret the ARGUMENT as a single value. This is trivial in the case of a reference or single value, which simply sets the future. However, it is of great utility in the case of nested functions, which can be treated as a single **already calculated** value. The program recursively launches asynchronous function calls to determine the resultant values as needed, and then executes the parent function as expected once they're all ready. This neatly solves any issue of control flow in waiting for results from an indeterminate number of nested function calls.

Future work includes further GUI improvements as well as further fleshing out the function cell type. The structure is already laid out to show the implementation of OOP principles used and demonstrates functionallity of the design structure. However, the parsing needs much more fleshing out to handle the various complexities of input form. This may get complicated enough to justify its own structure design just to make the parsing clear and sensible. Other parsing tools break up segments into "tokens", so I may look into prior work on that subject to use as a guide for my own implementation.