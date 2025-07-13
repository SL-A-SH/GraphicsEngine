#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

#include <QObject>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPoint>
#include <QMap>

class InputManager : public QObject
{
	Q_OBJECT

public:
	InputManager();
	~InputManager();

	bool Initialize(int screenWidth, int screenHeight);
	void Shutdown();
	bool Frame();

	void HandleKeyEvent(QKeyEvent* event, bool isPressed);
	void HandleMouseEvent(QMouseEvent* event);
	void HandleMouseMoveEvent(QMouseEvent* event);

	bool IsEscapePressed() const;
	bool IsLeftArrowPressed() const;
	bool IsRightArrowPressed() const;
	bool IsUpArrowPressed() const;
	bool IsDownArrowPressed() const;
	bool IsCtrlPressed() const;
	bool IsMousePressed() const;
	bool IsRightMousePressed() const;
	bool IsWPressed() const;
	bool IsAPressed() const;
	bool IsSPressed() const;
	bool IsDPressed() const;
	bool IsF11Pressed() const;
	bool IsF12Pressed() const;
	void GetMouseLocation(int& mouseX, int& mouseY) const;

private:
	void ProcessInput();

private:
	QMap<int, bool> m_keys;
	QMap<Qt::MouseButton, bool> m_mouseButtons;
	int m_mouseX;
	int m_mouseY;
	int m_screenWidth;
	int m_screenHeight;
};

#endif