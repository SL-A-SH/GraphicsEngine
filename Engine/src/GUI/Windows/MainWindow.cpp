#include "MainWindow.h"
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include "../../Core/System/Logger.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ViewportWidget(nullptr)
    , m_MainLayout(nullptr)
{
    // Set up the main window
    setWindowTitle("DirectX11 Engine");
    resize(1280, 720);
    
    // Create the central widget and layout
    QWidget* centralWidget = new QWidget(this);
    m_MainLayout = new QVBoxLayout(centralWidget);
    m_MainLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    // Create the DirectX viewport widget
    m_ViewportWidget = new DirectXViewport(centralWidget);
    m_ViewportWidget->setMinimumSize(800, 600);
    m_ViewportWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_MainLayout->addWidget(m_ViewportWidget);

    // Create UI elements
    CreateMenus();
    CreateToolbars();
    CreateDockWidgets();

    // Install event filter to handle F11
    installEventFilter(this);

    // Set initial window state to maximized
    showMaximized();

    LOG("Main window initialized successfully");
}

MainWindow::~MainWindow()
{
    LOG("Main window shutting down");
}

void MainWindow::CreateMenus()
{
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("File");
    
    QAction* openAction = new QAction("Open", this);
    fileMenu->addAction(openAction);
    
    QAction* saveAction = new QAction("Save", this);
    fileMenu->addAction(saveAction);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = new QAction("Exit", this);
    fileMenu->addAction(exitAction);
    
    // Edit menu
    QMenu* editMenu = menuBar()->addMenu("Edit");
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("View");
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("Help");
}

void MainWindow::CreateToolbars()
{
    // Create a toolbar
    QToolBar* mainToolBar = addToolBar("Main Toolbar");
    
    // Add some actions to the toolbar
    QAction* newAction = new QAction("New", this);
    mainToolBar->addAction(newAction);
    
    QAction* openAction = new QAction("Open", this);
    mainToolBar->addAction(openAction);
}

void MainWindow::CreateDockWidgets()
{
    // Create a dock widget for properties
    QDockWidget* propertiesDock = new QDockWidget("Properties", this);
    propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);
    
    // Set the initial size of the dock widget to 20% of the window width
    propertiesDock->setMinimumWidth(width() * 0.2);
    propertiesDock->setMaximumWidth(width() * 0.3);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_F11)
        {
            ToggleFullscreen();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::ToggleFullscreen()
{
    LOG("MainWindow::ToggleFullscreen called");
    if (isFullScreen())
    {
        LOG("Exiting fullscreen mode");
        showMaximized();  // Return to maximized state instead of normal
        // Restore UI elements
        menuBar()->show();
        for (auto toolbar : findChildren<QToolBar*>())
        {
            toolbar->show();
        }
        for (auto dock : findChildren<QDockWidget*>())
        {
            dock->show();
        }
    }
    else
    {
        LOG("Entering fullscreen mode");
        // Hide UI elements before going fullscreen
        menuBar()->hide();
        for (auto toolbar : findChildren<QToolBar*>())
        {
            toolbar->hide();
        }
        for (auto dock : findChildren<QDockWidget*>())
        {
            dock->hide();
        }
        showFullScreen();
    }
    
    // Ensure viewport gets focus
    m_ViewportWidget->setFocus();
    
    // Force a resize of the viewport
    m_ViewportWidget->resize(size());
} 