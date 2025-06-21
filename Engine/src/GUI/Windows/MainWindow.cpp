#include "MainWindow.h"
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QTabWidget>
#include <QTabBar>

#include "DirectXViewport.h"
#include "PerformanceWidget.h"
#include "../../Core/System/Logger.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_ViewportWidget(nullptr)
    , m_MainLayout(nullptr)
    , m_PerformanceWidget(nullptr)
    , m_TabWidget(nullptr)
    , m_PropertiesDock(nullptr)
{
    // Set up the main window
    setWindowTitle("DirectX11 Engine");
    resize(1280, 720);

    // Create UI elements
    CreateMenus();
    CreateToolbars();
    CreateDockWidgets();
    
    // Create the central widget and layout
    QWidget* centralWidget = new QWidget(this);
    m_MainLayout = new QVBoxLayout(centralWidget);
    m_MainLayout->setContentsMargins(0, 0, 0, 0);
    setCentralWidget(centralWidget);

    // Create tab widget
    m_TabWidget = new QTabWidget(centralWidget);
    m_TabWidget->setTabPosition(QTabWidget::North);
    m_TabWidget->setMovable(true);
    m_TabWidget->setTabsClosable(true); // Enable close button on tabs
    m_TabWidget->tabBar()->setVisible(false); // Hide tab bar initially
    m_MainLayout->addWidget(m_TabWidget);

    // Create the DirectX viewport widget
    m_ViewportWidget = new DirectXViewport(centralWidget);
    m_ViewportWidget->setMinimumSize(800, 600);
    m_ViewportWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_TabWidget->addTab(m_ViewportWidget, "Viewport");

    // Create performance widget
    m_PerformanceWidget = new PerformanceWidget(centralWidget);
    m_TabWidget->addTab(m_PerformanceWidget, "Performance");

    // Connect signals
    connect(m_TabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::OnTabCloseRequested);
    connect(m_TabWidget, &QTabWidget::currentChanged, this, &MainWindow::OnTabChanged);

    // Install event filters
    installEventFilter(this);
    qApp->installEventFilter(this);

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

    QAction* showFPSAction = new QAction("Show FPS", this);
    showFPSAction->setCheckable(true);  // Make it checkable
    showFPSAction->setChecked(false);   // Initially unchecked
    connect(showFPSAction, &QAction::toggled, this, &MainWindow::ToggleFPS);
    viewMenu->addAction(showFPSAction);
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("Tools");
    
    // Performance submenu
    QMenu* performanceMenu = toolsMenu->addMenu("Performance");
    
    QAction* benchmarkAction = new QAction("Run Benchmark", this);
    connect(benchmarkAction, &QAction::triggered, this, &MainWindow::RunBenchmark);
    performanceMenu->addAction(benchmarkAction);
    
    QAction* showProfilerAction = new QAction("Show Profiler", this);
    showProfilerAction->setCheckable(true);
    showProfilerAction->setChecked(false);
    connect(showProfilerAction, &QAction::toggled, this, &MainWindow::ToggleProfiler);
    performanceMenu->addAction(showProfilerAction);
    
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
    m_PropertiesDock = new QDockWidget("Properties", this);
    m_PropertiesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, m_PropertiesDock);
    
    // Set the initial size of the dock widget to 20% of the window width
    m_PropertiesDock->setMinimumWidth(width() * 0.2);
    m_PropertiesDock->setMaximumWidth(width() * 0.3);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event)
{
    // Handle F11 key press
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
        showMaximized();
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

void MainWindow::ToggleFPS(bool show)
{
    if (m_ViewportWidget && m_ViewportWidget->GetSystemManager() && 
        m_ViewportWidget->GetSystemManager()->GetApplication() &&
        m_ViewportWidget->GetSystemManager()->GetApplication()->GetUserInterface())
    {
        m_ViewportWidget->GetSystemManager()->GetApplication()->GetUserInterface()->SetShowFps(show);
    }
}

void MainWindow::RunBenchmark()
{
    // Show tab bar and enable performance tab
    m_TabWidget->tabBar()->setVisible(true);
    m_TabWidget->setCurrentIndex(1);
    LOG("Benchmark started");
}

void MainWindow::ToggleProfiler(bool show)
{
    if (show)
    {
        m_TabWidget->tabBar()->setVisible(true);
        m_TabWidget->setCurrentIndex(1);
    }
    else
    {
        m_TabWidget->setCurrentIndex(0);
        m_TabWidget->tabBar()->setVisible(false);
    }
}

void MainWindow::OnTabChanged(int index)
{
    // Show properties dock only when viewport tab is active
    if (m_PropertiesDock)
    {
        m_PropertiesDock->setVisible(index == 0);
    }
    
    // Set focus to the appropriate widget based on the active tab
    if (index == 0) // Viewport tab
    {
        m_ViewportWidget->setFocus();
    }
    else if (index == 1) // Performance tab
    {
        m_PerformanceWidget->setFocus();
    }
}

void MainWindow::OnTabCloseRequested(int index)
{
    if (index == 1) // Performance tab
    {
        m_TabWidget->setCurrentIndex(0);
        m_TabWidget->tabBar()->setVisible(false);
    }
}