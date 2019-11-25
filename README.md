# Spreadsheet
This file is a placeholder to show my current focus of study: the Gang of Four design principles.

The core functionality is held within the Cell and Table classes.
There is a little bit of interest in the implementation of SpreadsheetApplication.cpp, but it mostly just bridges Windows OS events/commands to Table public member functions.
The other files are fairly standard resources and infrastructure for a Windows application.

The Table header outlines an abstract base class TABLE to represent the GUI.
This model decouples the GUI implementation from the lower-level data management.
WINDOWS_TABLE inherets from TABLE to provide an implementation specific to a Windows environment.
It also defines a helper class CELL_ID, which aids in mapping a GUI cell to the appropraite cell data.
Only a Windows implementation is provided. This includes TABLE operations as well as additional functionality for the cell windows.

The Cell header defines a common base class for all cells as well as a factory for cell creation.
Each type of cell inherets from CELL and adds additional functionality as needed for its implementaiton.
The factory function creates the appropriate cell based off of user input and manages the data structure that holds cell data.
This function also produces notifications so that cells know when data they reference is changed.
As of this writing, only text, number, and reference cells have been made.

A reference is made starting with the '&' character followed by "R___C___" filling in the appropriate row & collumn numbers.
(Case and order do not matter. Adding other characters beyond this will produce invalid input and throw an error.)
Reference cells should update as the referenced cell is changed.
Dangling references throw an error: "!REF!"

Future work includes further GUI functionality as well as a function cell type.
Function cells will hold one or more functions, which take zero or more arguments.
(The strategy patern will encapsulate functions into interchangable objects.)
(The composite pattern will group function objects and arguments to treat singles and groups uniformly.)
