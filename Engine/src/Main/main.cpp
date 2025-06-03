#include "../Core/System/SystemManager.h"
#include "../GUI/Windows/MainWindow.h"
#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
	// Initialize Qt Application
	QApplication app(argc, argv);

	// Create the system object
	SystemManager* System = new SystemManager;

	// Initialize the system
	bool result = System->Initialize();
	if (!result)
	{
		delete System;
		return -1;
	}

	// Create and show the main window
	MainWindow mainWindow;
	mainWindow.show();

	// Run the Qt event loop
	int returnCode = app.exec();

	// Shutdown and release the system object
	System->Shutdown();
	delete System;

	return returnCode;
}