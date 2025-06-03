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
    // Set up the widget to receive native events
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_DontCreateNativeAncestors);

    // Create system manager
    m_SystemManager = new SystemManager();
}

DirectXViewport::~DirectXViewport()
{
    if (m_UpdateTimer)
    {
        m_UpdateTimer->stop();
        delete m_UpdateTimer;
    }

    if (m_SystemManager)
    {
        m_SystemManager->Shutdown();
        delete m_SystemManager;
    }
}

void DirectXViewport::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    
    if (!m_Initialized)
    {
        // Ensure we have a valid window handle
        if (!winId())
        {
            LOG_ERROR("No valid window handle available");
            return;
        }

        // Set the window handle
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

        if (!m_SystemManager->Initialize())
        {
            LOG_ERROR("Failed to initialize SystemManager");
            return;
        }

        // Create update timer
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
    QWidget::resizeEvent(event);
    if (m_Initialized)
    {
        std::stringstream ss;
        ss << "Viewport resized to: " << width() << "x" << height();
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

void DirectXViewport::updateFrame()
{
    if (m_Initialized && m_SystemManager)
    {
        m_SystemManager->Frame();
        update(); // Request a repaint
    }
} 