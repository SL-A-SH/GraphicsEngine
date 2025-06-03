#include "MainWindow.h"
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QFileDialog>

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
    setCentralWidget(centralWidget);

    // Create the viewport widget (this will be replaced with your DirectX11 viewport)
    m_ViewportWidget = new QWidget(centralWidget);
    m_ViewportWidget->setMinimumSize(800, 600);
    m_MainLayout->addWidget(m_ViewportWidget);

    // Create UI elements
    CreateMenus();
    CreateToolbars();
    CreateDockWidgets();
}

MainWindow::~MainWindow()
{
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
    // Create a dock widget for the scene hierarchy
    QDockWidget* sceneDock = new QDockWidget("Scene", this);
    sceneDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    
    // Create a dock widget for properties
    QDockWidget* propertiesDock = new QDockWidget("Properties", this);
    propertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, propertiesDock);
} 