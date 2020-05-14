This project is meant to demonstrate one specific focus of study: the Gang of Four design principles. It is intended to be a spreadsheet application similar to Microsoft Excel. It is reasonably functional in it's current state. The spreadsheet can currently handle text, numbers, and active references properly. In progress are functions, which only offers a few functions and can handle modest complexity. Parsing has not yet been made to handle operators (+,-,*,/), nor can it handle () for order of operations. However, it can handle recursive composition, live update of reference arguments, and extraneous spaces (i.e. =AVERAGE(1, &R1C1, 3, SUM(4, 5), 6)). I may need to rethink the parsing scheme to use an object-oriented rather than procedural approach to make the various complexities easier to handle.

The core functionality is held within the Cell and Table classes. The file SpreadsheetApplication.cpp simply maps Windows OS events/commands to TABLE public member functions. WINDOWS_TABLE then serves as an "Adapter" pattern, which ties the OS specific interface to a generic interface against which the underlying program logic is designed. The other files are fairly standard resources and infrastructure for a Windows application.

The Table header outlines an abstract base class TABLE to represent the GUI. This model decouples the GUI implementation from the lower-level data management. WINDOWS_TABLE inherets from TABLE to provide an implementation specific to a Windows environment. It also defines a helper class CELL_ID, which aids in mapping a GUI cell to the appropraite cell data. By default, a base-level Windows implementation is provided. This includes TABLE operations as well as additional functionality for the cell windows. Other implementaitons, for Windows or other OS's, could easily be added since proper decoupling is utilized.

A new console interface is presently being developed to demonstrate the interchangability of the interface. To switch over to that, change the compiler flag _WINDOWS -> _CONSOLE and change the linker subsystem WINDOWS -> CONSOLE. (Project  Properties -> Linker -> System -> SubSystem) The change has been verified to successfully compile into a console application rather than a Windows GUI application. That said, no functionallity has yet been developed for console mode.


CELL_ID demonstrates the "Builder" pattern to allow for clear, fluent usage. One problem this solves is mixing up constructor arguments. Rows & columns could easily be flipped and it may be hard to back track such errors. By using a builder pattern, the client programmer must clearly state each assignment as either a row or column. Mistakes will be minimized and may stand out more clearly with such clear syntax. A fluent model is also used so that the programmer may smoothly chain together member function calls that are conceptually related. (I.e. CELL_ID.SetRow().SetColumn();)

The Cell header defines a common base class for all cells as well as implementing a "Factory" pattern for cell creation. (All CELLs share the same factory, which corresponds to the "Singleton" pattern as well.) Each type of cell inherets from CELL and adds additional functionality as needed for its implementaiton. The factory function creates the appropriate cell based off of user input and manages the data structure that holds all cell data. This function also produces notifications so that cells know that data they reference has changed. Each cell edit deletes the old cell and creates it anew to change cell types and push updates as needed. As of this writing, the types of CELL's are: text, numerical, reference, and function. The function cells, being the most complicated, are still being fleshed out fully.

Further aiding notifications, a CELL_PROXY class was created, which implements the "Proxy" pattern. This gives users and derived classes only indirect access to the CELLs they use. By doing so, CELL can intercept any changes and trigger a notification through CELL_FACTORY. The proxy is similar to a smart pointer in that it forwards the member access operator ->() and is convertable to bool to check for null values. A user should be able to use the proxy as if it were the real thing while also triggering update notifications automagically.

A text cell is the default cell type. Any cell that starts with anything beyond a number, '&' for a reference, or '=' for a function will be a text cell. Further, if the intended type is ambiguous, the factory will enforce interpretation as text (ex. 123ABC). As with other types, interpretation as text can be enforce by prepending the ''' character. No error should occur here since input text can always be interpreted as text.

A numerical cell starts with a number, decimal, or negative sign. The factory will call std::stod to reinterpret the text as a double and store that value. However, this library function is more forgiving than is appropriate here, so a manual check is done for alphabetical characters since they should not occur in a valid decimal number. Any error here causes the factory to fall back upon a text interpretation, which should always succeed.

A reference is made starting with the '&' character followed by "R___C___" filling in the appropriate row & column numbers. (Case and order do not matter. Adding other characters beyond this will produce invalid input and throw an error.) Reference cells update as the referenced cell is changed by utilizing an "Observer" pattern. The cell factory function subscribes and unsubscribes reference cells as they are created and destroyed. Dangling references throw an error: "!REF!"

In Progress: Function Cells
Function cells are created by starting with '='. A function cell is a cell that contains a single function, a vector of arguments, and a single result for display. Each argument may be either a single value, a reference to another cell, or another function. As such, functions can be recursively composed to contain any number of sub-functions. Function cells make use of the "Strategy" pattern to allow for a multiplicity of functionallities utilizing a consistent interface. (Each FUCNTION object has an operation it performs on its vector of ARGUMENTS to produce a single result, which itself is an ARGUMENT to enable recursion.) This option of specializing a generic base function object for each operation is selected to fit the goal of demonstrating a variety of OOP design patterns. One possible alternative is to store a function pointer that can be assigned the appropraite function (such as a lambda or other callable object) as opposed to creating a subclass for each possible function the program recognizes.

The ARGUMENT object used by FUNCTIONs makes use of the "Composite" design pattern. This allows a FUNCTION to treat all arguments as a single value, ignoring any underlying complexity. A FUNCTION simply calls .get() on the stored future to interpret the ARGUMENT as a single value. This is trivial in the case of a reference or single value, which simply sets the future. However, it is of great utility in the case of nested functions, which can be treated as a single **already calculated** value. The program recursively launches asynchronous function calls to determine the resultant values as needed, and then executes the parent function as expected once they're all ready. This neatly solves any issue of control flow in waiting for results from an indeterminate number of nested function calls.

Future work includes further GUI improvements as well as further developing the function cell type. The structure is already laid out to show the implementation of OOP principles used and demonstrates functionallity of the design structure. However, the parsing of functions can get very convoluted and needs further fleshing out to handle the various complexities of input form. This may get tangled enough to justify its own structure design just to make the parsing clear and sensible. Other parsing tools break up segments into "tokens", so I may look into prior work on that subject to use as a guide for my own implementation. I also need to figure out the best way to map strings representing function names to their corresponding objects. Also under consideration is a save/load feature.