#include "InputManager.h"
#include <windows.h>
#include <QKeyEvent>
#include <QMouseEvent>
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

void InputManager::HandleKeyEvent(QKeyEvent* event, bool pressed)
{
	/*LOG("InputManager::HandleKeyEvent called");*/
	int key = event->key();
	/*LOG("Key: " + std::to_string(key) + " Pressed: " + std::to_string(pressed));*/

	switch (key)
	{
		case Qt::Key_W:
			m_keys[Qt::Key_W] = pressed;
			LOG("W key state: " + std::to_string(pressed));
			break;
		case Qt::Key_A:
			m_keys[Qt::Key_A] = pressed;
			LOG("A key state: " + std::to_string(pressed));
			break;
		case Qt::Key_S:
			m_keys[Qt::Key_S] = pressed;
			LOG("S key state: " + std::to_string(pressed));
			break;
		case Qt::Key_D:
			m_keys[Qt::Key_D] = pressed;
			LOG("D key state: " + std::to_string(pressed));
			break;
		case Qt::Key_F11:
			m_keys[Qt::Key_F11] = pressed;
			LOG("F11 key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Escape:
			m_keys[Qt::Key_Escape] = pressed;
			LOG("Escape key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Control:
			m_keys[Qt::Key_Control] = pressed;
			LOG("Control key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Left:
			m_keys[Qt::Key_Left] = pressed;
			LOG("Left arrow key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Right:
			m_keys[Qt::Key_Right] = pressed;
			LOG("Right arrow key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Up:
			m_keys[Qt::Key_Up] = pressed;
			LOG("Up arrow key state: " + std::to_string(pressed));
			break;
		case Qt::Key_Down:
			m_keys[Qt::Key_Down] = pressed;
			LOG("Down arrow key state: " + std::to_string(pressed));
			break;
	}
}

void InputManager::HandleMouseEvent(QMouseEvent* event)
{
	LOG("InputManager::HandleMouseEvent called");
	Qt::MouseButton button = event->button();
	bool pressed = (event->type() == QEvent::MouseButtonPress);
	LOG("Mouse button: " + std::to_string(button) + " Pressed: " + std::to_string(pressed));

	switch (button)
	{
		case Qt::RightButton:
			m_mouseButtons[Qt::RightButton] = pressed;
			LOG("Right mouse button state: " + std::to_string(pressed));
			break;
		case Qt::LeftButton:
			m_mouseButtons[Qt::LeftButton] = pressed;
			LOG("Left mouse button state: " + std::to_string(pressed));
			break;
	}
}

void InputManager::HandleMouseMoveEvent(QMouseEvent* event)
{
	LOG("InputManager::HandleMouseMoveEvent called");
	m_mouseX = event->x();
	m_mouseY = event->y();
	LOG("Mouse position: " + std::to_string(m_mouseX) + ", " + std::to_string(m_mouseY));
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

bool InputManager::IsWPressed() const
{
	return m_keys.value(Qt::Key_W, false);
}

bool InputManager::IsAPressed() const
{
	return m_keys.value(Qt::Key_A, false);
}

bool InputManager::IsSPressed() const
{
	return m_keys.value(Qt::Key_S, false);
}

bool InputManager::IsDPressed() const
{
	return m_keys.value(Qt::Key_D, false);
}

bool InputManager::IsF11Pressed() const
{
	return m_keys.value(Qt::Key_F11, false);
}