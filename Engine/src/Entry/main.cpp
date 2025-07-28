#include <QApplication>
#include <QWidget>

#include "../GUI/Windows/MainWindow.h"
#include "../GUI/Windows/ThemeManager.h"
#include "../Core/System/Logger.h"

int main(int argc, char *argv[])
{
	// Initialize logger
	Logger::GetInstance().Initialize();

	// Initialize Qt Application
	QApplication app(argc, argv);

	// Apply dark mode styling
	ThemeManager::ApplyDarkTheme(app);

	// Create and show the main window
	MainWindow mainWindow;
	mainWindow.show();

	LOG("Application started successfully with dark mode");

	// Run the Qt event loop
	int returnCode = app.exec();

	LOG("Application shutting down");

	return returnCode;
}