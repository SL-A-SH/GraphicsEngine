#include "../Core/System/SystemManager.h"
#include "../GUI/Windows/MainWindow.h"
#include "../Core/System/Logger.h"
#include <QApplication>
#include <QWidget>

int main(int argc, char *argv[])
{
	// Initialize logger
	Logger::GetInstance().Initialize();

	// Initialize Qt Application
	QApplication app(argc, argv);

	// Create and show the main window
	MainWindow mainWindow;
	mainWindow.show();

	LOG("Application started successfully");

	// Run the Qt event loop
	int returnCode = app.exec();

	LOG("Application shutting down");

	return returnCode;
}