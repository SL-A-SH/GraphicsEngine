#include "DirectXViewport.h"
#include <sstream>

#include "../../Core/System/Logger.h"

DirectXViewport::DirectXViewport(QWidget* parent)
    : QWidget(parent)
    , m_SystemManager(nullptr)
    , m_UpdateTimer(nullptr)
    , m_Initialized(false)
{
    LOG("DirectXViewport constructor called");
    
    // Log widget hierarchy and geometry
    LOG("Widget parent: " + (parent ? parent->objectName().toStdString() : "null"));
    LOG("Widget geometry: " + std::to_string(geometry().x()) + "," + 
        std::to_string(geometry().y()) + " " +
        std::to_string(width()) + "x" + std::to_string(height()));
    
    // Set up the widget to receive native events
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DontCreateNativeAncestors);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_ShowWithoutActivating);

    // Enable focus and input handling
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    setFocus();

    // Ensure the widget can receive mouse events
    setAttribute(Qt::WA_AcceptTouchEvents, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    
    // Force the widget to be on top and receive events
    raise();
    activateWindow();
    
    // Set a minimum size to ensure the widget is visible
    setMinimumSize(100, 100);

    // Create system manager
    LOG("Creating SystemManager");
    m_SystemManager = new SystemManager();
    if (!m_SystemManager->Initialize())
    {
        LOG_ERROR("Failed to initialize SystemManager");
        delete m_SystemManager;
        m_SystemManager = nullptr;
    }
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
    
    // Check for widgets overlapping our viewport
    if (parentWidget()) {
        QPoint globalPos = mapToGlobal(rect().center());
        QWidget* child = QApplication::widgetAt(globalPos);
        if (child && child != this) {
            LOG("WARNING: Another widget is overlapping our viewport: " + child->objectName().toStdString());
        }
    }

    // Check native window status
    if (!testAttribute(Qt::WA_WState_Created)) {
        LOG("ERROR: Widget native window not created");
    }
    if (!internalWinId()) {
        LOG("ERROR: No native window ID");
    }

    // Bring to front and request focus
    raise();
    activateWindow();
    setFocus();
    
    // Force the widget to be visible and on top
    show();
    setWindowState(windowState() | Qt::WindowActive);
    
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
        if (m_SystemManager)
        {
            m_SystemManager->SetWindowHandle((HWND)winId());
            
            // Wait a moment for the Application to initialize
            QApplication::processEvents();
            
            // Verify that the Application was initialized successfully
            if (!m_SystemManager->GetApplication())
            {
                LOG_ERROR("Application failed to initialize after setting window handle");
                return;
            }
        }

        // Get the monitor's refresh rate
        DEVMODE devMode;
        devMode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devMode);
        int refreshRate = devMode.dmDisplayFrequency;
        
        // Create update timer with the monitor's refresh rate
        LOG("Creating update timer");
        m_UpdateTimer = new QTimer(this);
        connect(m_UpdateTimer, &QTimer::timeout, this, &DirectXViewport::updateFrame);
        m_UpdateTimer->start(1000 / refreshRate); // Use monitor's refresh rate

        m_Initialized = true;
        LOG("DirectX viewport initialized successfully");
    }
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
    if (eventType == "windows_generic_MSG")
    {
        MSG* msg = static_cast<MSG*>(message);
        
        // Handle mouse events first
        if (msg->message >= WM_MOUSEFIRST && msg->message <= WM_MOUSELAST)
        {
            // Convert Windows mouse message to Qt mouse event
            QMouseEvent* mouseEvent = nullptr;
            QPoint pos(LOWORD(msg->lParam), HIWORD(msg->lParam));
            
            switch (msg->message)
            {
                case WM_LBUTTONDOWN:
                    mouseEvent = new QMouseEvent(QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                    break;
                case WM_LBUTTONUP:
                    mouseEvent = new QMouseEvent(QEvent::MouseButtonRelease, pos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                    break;
                case WM_RBUTTONDOWN:
                    mouseEvent = new QMouseEvent(QEvent::MouseButtonPress, pos, Qt::RightButton, Qt::RightButton, Qt::NoModifier);
                    break;
                case WM_RBUTTONUP:
                    mouseEvent = new QMouseEvent(QEvent::MouseButtonRelease, pos, Qt::RightButton, Qt::NoButton, Qt::NoModifier);
                    break;
                case WM_MOUSEMOVE:
                    mouseEvent = new QMouseEvent(QEvent::MouseMove, pos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                    break;
            }
            
            if (mouseEvent)
            {
                QApplication::sendEvent(this, mouseEvent);
                delete mouseEvent;
            }
        }
        
        // Then pass to SystemManager
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
    LOG("DirectXViewport::keyPressEvent called - Key: " + QString::number(event->key()).toStdString());
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleKeyEvent(event, true);
    }
    event->accept();  // Explicitly accept the event
    QWidget::keyPressEvent(event);
}

void DirectXViewport::keyReleaseEvent(QKeyEvent* event)
{
    LOG("DirectXViewport::keyReleaseEvent called - Key: " + QString::number(event->key()).toStdString());
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleKeyEvent(event, false);
    }
    event->accept();  // Explicitly accept the event
    QWidget::keyReleaseEvent(event);
}

void DirectXViewport::mousePressEvent(QMouseEvent* event)
{
    LOG("DirectXViewport::mousePressEvent called - Button: " + QString::number(event->button()).toStdString());
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseEvent(event);
        
        // Capture mouse on right click for camera rotation
        if (event->button() == Qt::RightButton)
        {
            LOG("Capturing mouse for camera rotation");
            setMouseTracking(true);
            grabMouse();
        }
    }
    event->accept();  // Explicitly accept the event
    QWidget::mousePressEvent(event);
}

void DirectXViewport::mouseReleaseEvent(QMouseEvent* event)
{
    LOG("DirectXViewport::mouseReleaseEvent called - Button: " + QString::number(event->button()).toStdString());
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseEvent(event);
        
        // Release mouse capture when right button is released
        if (event->button() == Qt::RightButton)
        {
            LOG("Releasing mouse capture");
            releaseMouse();
        }
    }
    event->accept();  // Explicitly accept the event
    QWidget::mouseReleaseEvent(event);
}

void DirectXViewport::mouseMoveEvent(QMouseEvent* event)
{
    LOG("DirectXViewport::mouseMoveEvent called - Position: " + 
        QString::number(event->pos().x()).toStdString() + "," + 
        QString::number(event->pos().y()).toStdString());
    if (m_SystemManager && m_SystemManager->GetInputManager())
    {
        m_SystemManager->GetInputManager()->HandleMouseMoveEvent(event);
    }
    event->accept();  // Explicitly accept the event
    QWidget::mouseMoveEvent(event);
}

void DirectXViewport::updateFrame()
{
    if (m_Initialized && m_SystemManager)
    {
        // Additional check to ensure the Application is properly initialized
        if (m_SystemManager->GetApplication())
        {
            m_SystemManager->Frame();
        }
        else
        {
            LOG_WARNING("Application not yet initialized, skipping frame");
        }
        // Don't call update() here as it triggers Qt's paint system
    }
    else
    {
        LOG_WARNING("DirectXViewport not yet initialized, skipping frame");
    }
}

void DirectXViewport::focusInEvent(QFocusEvent* event)
{
    LOG("DirectXViewport GOT FOCUS");
    QWidget::focusInEvent(event);
}

void DirectXViewport::focusOutEvent(QFocusEvent* event)
{
    LOG("DirectXViewport LOST FOCUS");
    QWidget::focusOutEvent(event);
} 

bool DirectXViewport::event(QEvent* event)
{
    QString eventType;
    switch (event->type()) {
        case QEvent::MouseButtonPress: eventType = "MouseButtonPress"; break;
        case QEvent::MouseButtonRelease: eventType = "MouseButtonRelease"; break;
        case QEvent::MouseButtonDblClick: eventType = "MouseButtonDblClick"; break;
        case QEvent::MouseMove: eventType = "MouseMove"; break;
        case QEvent::Enter: eventType = "Enter"; break;
        case QEvent::Leave: eventType = "Leave"; break;
        case QEvent::HoverMove: eventType = "HoverMove"; break;
        case QEvent::HoverEnter: eventType = "HoverEnter"; break;
        case QEvent::HoverLeave: eventType = "HoverLeave"; break;
        case QEvent::Wheel: eventType = "Wheel"; break;
        default: return QWidget::event(event);
    }
    
    LOG("Event received: " + eventType.toStdString());
    return QWidget::event(event);
}