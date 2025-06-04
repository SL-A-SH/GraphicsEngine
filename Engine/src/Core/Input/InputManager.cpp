#include "InputManager.h"
#include "../../Core/System/Logger.h"

InputManager::InputManager()
{
	m_mouseX = 0;
	m_mouseY = 0;
	m_screenWidth = 0;
	m_screenHeight = 0;
}

InputManager::~InputManager()
{
}

bool InputManager::Initialize(int screenWidth, int screenHeight)
{
	m_screenWidth = screenWidth;
	m_screenHeight = screenHeight;
	m_mouseX = screenWidth / 2;
	m_mouseY = screenHeight / 2;

	LOG("InputManager initialized successfully");
	return true;
}

void InputManager::Shutdown()
{
}

bool InputManager::Frame()
{
	return true;
}

void InputManager::HandleKeyEvent(QKeyEvent* event, bool isPressed)
{
	if (!event) return;
	
	int key = event->key();
	m_keys[key] = isPressed;
}

void InputManager::HandleMouseEvent(QMouseEvent* event)
{
	if (!event) return;

	Qt::MouseButton button = event->button();
	bool isPressed = (event->type() == QEvent::MouseButtonPress);
	
	m_mouseButtons[button] = isPressed;
	m_mouseX = event->x();
	m_mouseY = event->y();
}

void InputManager::HandleMouseMoveEvent(QMouseEvent* event)
{
	if (!event) return;
	
	m_mouseX = event->x();
	m_mouseY = event->y();
}

void InputManager::GetMouseLocation(int& mouseX, int& mouseY) const
{
	mouseX = m_mouseX;
	mouseY = m_mouseY;
}

bool InputManager::IsEscapePressed() const
{
	return m_keys.value(Qt::Key_Escape, false);
}

bool InputManager::IsLeftArrowPressed() const
{
	return m_keys.value(Qt::Key_Left, false);
}

bool InputManager::IsRightArrowPressed() const
{
	return m_keys.value(Qt::Key_Right, false);
}

bool InputManager::IsUpArrowPressed() const
{
	return m_keys.value(Qt::Key_Up, false);
}

bool InputManager::IsDownArrowPressed() const
{
	return m_keys.value(Qt::Key_Down, false);
}

bool InputManager::IsCtrlPressed() const
{
	return m_keys.value(Qt::Key_Control, false);
}

bool InputManager::IsMousePressed() const
{
	return m_mouseButtons.value(Qt::LeftButton, false);
}

bool InputManager::IsRightMousePressed() const
{
	return m_mouseButtons.value(Qt::RightButton, false);
}