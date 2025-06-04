#include "DirectXViewport.h"
#include <QPainter>
#include <QResizeEvent>
#include <QApplication>
#include <sstream>
#include "../../Core/System/Logger.h"

DirectXViewport::DirectXViewport(QWidget* parent)
    : QWidget(parent)
    , m_SystemManager(nullptr)
    , m_UpdateTimer(nullptr)
    , m_Initialized(false)
{
    LOG("DirectXViewport constructor called");
    
    // Set up the widget to receive native events
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Enable mouse tracking for continuous mouse move events
    setMouseTracking(true);

    // Create system manager
    LOG("Creating SystemManager");
    m_SystemManager = new SystemManager();
}

DirectXViewport::~DirectXViewport()
{
    LOG("DirectXViewport destructor called");
    if (m_UpdateTimer)
    {
        LOG("Stopping update timer");
        m_UpdateTimer->stop();
        delete m_UpdateTimer;
        m_UpdateTimer = nullptr;
    }

    if (m_SystemManager)
    {
        LOG("Shutting down SystemManager");
        m_SystemManager->Shutdown();
        delete m_SystemManager;
        m_SystemManager = nullptr;
    }
}

void DirectXViewport::showEvent(QShowEvent* event)
{
    LOG("DirectXViewport::showEvent called");
    QWidget::showEvent(event);
    
    if (!m_Initialized)
    {
        // Ensure we have a valid window handle
        if (!winId())
        {
            LOG_ERROR("No valid window handle available");
            return;
        }

        // Wait for the window to be fully created
        QApplication::processEvents();

        // Set the window handle
        LOG("Setting window handle");
        m_SystemManager->SetWindowHandle((HWND)winId());

        // Get the actual window size
        RECT rect;
        GetClientRect((HWND)winId(), &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        if (width == 0 || height == 0)
        {
            std::stringstream ss;
            ss << "Invalid window size: " << width << "x" << height;
            LOG_ERROR(ss.str());
            return;
        }

        // Ensure the window is fully created and visible
        LOG("Showing window");
        ShowWindow((HWND)winId(), SW_SHOW);
        UpdateWindow((HWND)winId());

        LOG("Initializing SystemManager");
        if (!m_SystemManager->Initialize())
        {
            LOG_ERROR("Failed to initialize SystemManager");
            return;
        }

        // Create update timer with a more appropriate interval (60 FPS)
        LOG("Creating update timer");
        m_UpdateTimer = new QTimer(this);
        connect(m_UpdateTimer, &QTimer::timeout, this, &DirectXViewport::updateFrame);
        m_UpdateTimer->start(16); // ~60 FPS

        m_Initialized = true;
        std::stringstream ss;
        ss << "DirectX viewport initialized successfully with size: " << width << "x" << height;
        LOG(ss.str());
    }
}

void DirectXViewport::paintEvent(QPaintEvent* event)
{
    // Let Qt know we're handling the painting
    Q_UNUSED(event);
}

void DirectXViewport::resizeEvent(QResizeEvent* event)
{
    LOG("DirectXViewport::resizeEvent called");
    QWidget::resizeEvent(event);
    
    if (m_Initialized && m_SystemManager)
    {
        // Get the actual window size
        RECT rect;
        GetClientRect((HWND)winId(), &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // Update the DirectX viewport
        if (m_SystemManager->GetApplication())
        {
            LOG("Resizing application");
            m_SystemManager->GetApplication()->Resize(width, height);
        }

        std::stringstream ss;
        ss << "Viewport resized to: " << width << "x" << height;
        LOG(ss.str());
    }
}

bool DirectXViewport::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    // Forward Windows messages to the system manager
    if (eventType == "windows_generic_MSG")
    {
        MSG* msg = static_cast<MSG*>(message);
        if (m_SystemManager)
        {
            *result = m_SystemManager->MessageHandler(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            return true;
        }
    }
    return false;
}

void DirectXViewport::keyPressEvent(QKeyEvent* event)
{
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleKeyEvent(event, true);
    }
    QWidget::keyPressEvent(event);
}

void DirectXViewport::keyReleaseEvent(QKeyEvent* event)
{
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleKeyEvent(event, false);
    }
    QWidget::keyReleaseEvent(event);
}

void DirectXViewport::mousePressEvent(QMouseEvent* event)
{
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseEvent(event);
    }
    QWidget::mousePressEvent(event);
}

void DirectXViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseEvent(event);
    }
    QWidget::mouseReleaseEvent(event);
}

void DirectXViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseMoveEvent(event);
    }
    QWidget::mouseMoveEvent(event);
}

void DirectXViewport::updateFrame()
{
    if (m_Initialized && m_SystemManager)
    {
        LOG("DirectXViewport::updateFrame called");
        m_SystemManager->Frame();
        // Don't call update() here as it triggers Qt's paint system
    }
} 