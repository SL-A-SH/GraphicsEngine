#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

class InputManager
{
public:
	InputManager();
	InputManager(const InputManager&);
	~InputManager();

	void Initialize();

	void KeyDown(unsigned int);
	void KeyUp(unsigned int);

	bool IsKeyDown(unsigned int);

private:
	bool m_keys[256];
};

#endif