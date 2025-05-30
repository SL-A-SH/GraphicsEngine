#include "position.h"

Position::Position()
{
    m_frameTime = 0.0f;
    m_rotationY = 0.0f;
    m_rotationX = 0.0f;
    m_positionX = 0.0f;
    m_positionY = 0.0f;
    m_positionZ = 0.0f;
    m_leftTurnSpeed = 0.0f;
    m_rightTurnSpeed = 0.0f;
    m_upTurnSpeed = 0.0f;
    m_downTurnSpeed = 0.0f;
    m_forwardMoveSpeed = 0.0f;
    m_backwardMoveSpeed = 0.0f;
    m_leftMoveSpeed = 0.0f;
    m_rightMoveSpeed = 0.0f;
    m_upMoveSpeed = 0.0f;
    m_downMoveSpeed = 0.0f;
}


Position::Position(const Position& other)
{
}


Position::~Position()
{
}


void Position::SetFrameTime(float time)
{
    m_frameTime = time;
    return;
}


void Position::GetRotation(float& y)
{
    y = m_rotationY;
    return;
}


void Position::LookLeft(bool keydown)
{
    // If the key is pressed increase the speed at which the camera turns left.  If not slow down the turn speed.
    if (keydown)
    {
        m_leftTurnSpeed += m_frameTime * 1.5f;

        if (m_leftTurnSpeed > (m_frameTime * 200.0f))
        {
            m_leftTurnSpeed = m_frameTime * 200.0f;
        }
    }
    else
    {
        m_leftTurnSpeed -= m_frameTime * 1.0f;

        if (m_leftTurnSpeed < 0.0f)
        {
            m_leftTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationY -= m_leftTurnSpeed;
    if (m_rotationY < 0.0f)
    {
        m_rotationY += 360.0f;
    }

    return;
}


void Position::LookRight(bool keydown)
{
    // If the key is pressed increase the speed at which the camera turns right.  If not slow down the turn speed.
    if (keydown)
    {
        m_rightTurnSpeed += m_frameTime * 1.5f;

        if (m_rightTurnSpeed > (m_frameTime * 200.0f))
        {
            m_rightTurnSpeed = m_frameTime * 200.0f;
        }
    }
    else
    {
        m_rightTurnSpeed -= m_frameTime * 1.0f;

        if (m_rightTurnSpeed < 0.0f)
        {
            m_rightTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationY += m_rightTurnSpeed;
    if (m_rotationY > 360.0f)
    {
        m_rotationY -= 360.0f;
    }

    return;
}

void Position::GetRotationX(float& x)
{
    x = m_rotationX;
    return;
}

void Position::GetPosition(float& x, float& y, float& z)
{
    x = m_positionX;
    y = m_positionY;
    z = m_positionZ;
    return;
}

void Position::LookUp(bool keydown)
{
    // If the key is pressed increase the speed at which the camera looks up.  If not slow down the turn speed.
    if (keydown)
    {
        m_upTurnSpeed += m_frameTime * 1.5f;

        if (m_upTurnSpeed > (m_frameTime * 200.0f))
        {
            m_upTurnSpeed = m_frameTime * 200.0f;
        }
    }
    else
    {
        m_upTurnSpeed -= m_frameTime * 1.0f;

        if (m_upTurnSpeed < 0.0f)
        {
            m_upTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationX -= m_upTurnSpeed;
    if (m_rotationX < -90.0f)
    {
        m_rotationX = -90.0f;
    }

    return;
}

void Position::LookDown(bool keydown)
{
    // If the key is pressed increase the speed at which the camera looks down.  If not slow down the turn speed.
    if (keydown)
    {
        m_downTurnSpeed += m_frameTime * 1.5f;

        if (m_downTurnSpeed > (m_frameTime * 200.0f))
        {
            m_downTurnSpeed = m_frameTime * 200.0f;
        }
    }
    else
    {
        m_downTurnSpeed -= m_frameTime * 1.0f;

        if (m_downTurnSpeed < 0.0f)
        {
            m_downTurnSpeed = 0.0f;
        }
    }

    // Update the rotation using the turning speed.
    m_rotationX += m_downTurnSpeed;
    if (m_rotationX > 90.0f)
    {
        m_rotationX = 90.0f;
    }

    return;
}

void Position::MoveForward(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves forward.  If not slow down the move speed.
    if (keydown)
    {
        m_forwardMoveSpeed += m_frameTime * 1.0f;

        if (m_forwardMoveSpeed > (m_frameTime * 50.0f))
        {
            m_forwardMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_forwardMoveSpeed -= m_frameTime * 0.5f;

        if (m_forwardMoveSpeed < 0.0f)
        {
            m_forwardMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionX += sinf(radians) * m_forwardMoveSpeed;
    m_positionZ += cosf(radians) * m_forwardMoveSpeed;

    return;
}

void Position::MoveBackward(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves backward.  If not slow down the move speed.
    if (keydown)
    {
        m_backwardMoveSpeed += m_frameTime * 1.0f;

        if (m_backwardMoveSpeed > (m_frameTime * 50.0f))
        {
            m_backwardMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_backwardMoveSpeed -= m_frameTime * 0.5f;

        if (m_backwardMoveSpeed < 0.0f)
        {
            m_backwardMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionX -= sinf(radians) * m_backwardMoveSpeed;
    m_positionZ -= cosf(radians) * m_backwardMoveSpeed;

    return;
}

void Position::MoveLeft(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves left.  If not slow down the move speed.
    if (keydown)
    {
        m_leftMoveSpeed += m_frameTime * 1.0f;

        if (m_leftMoveSpeed > (m_frameTime * 50.0f))
        {
            m_leftMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_leftMoveSpeed -= m_frameTime * 0.5f;

        if (m_leftMoveSpeed < 0.0f)
        {
            m_leftMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionX -= cosf(radians) * m_leftMoveSpeed;
    m_positionZ += sinf(radians) * m_leftMoveSpeed;

    return;
}

void Position::MoveRight(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves right.  If not slow down the move speed.
    if (keydown)
    {
        m_rightMoveSpeed += m_frameTime * 1.0f;

        if (m_rightMoveSpeed > (m_frameTime * 50.0f))
        {
            m_rightMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_rightMoveSpeed -= m_frameTime * 0.5f;

        if (m_rightMoveSpeed < 0.0f)
        {
            m_rightMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionX += cosf(radians) * m_rightMoveSpeed;
    m_positionZ -= sinf(radians) * m_rightMoveSpeed;

    return;
}

void Position::MoveUp(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves up.  If not slow down the move speed.
    if (keydown)
    {
        m_upMoveSpeed += m_frameTime * 1.0f;

        if (m_upMoveSpeed > (m_frameTime * 50.0f))
        {
            m_upMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_upMoveSpeed -= m_frameTime * 0.5f;

        if (m_upMoveSpeed < 0.0f)
        {
            m_upMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionY += cosf(radians) * m_upMoveSpeed;
    m_positionZ -= sinf(radians) * m_upMoveSpeed;

    return;
}

void Position::MoveDown(bool keydown)
{
    float radians;

    // If the key is pressed increase the speed at which the camera moves up.  If not slow down the move speed.
    if (keydown)
    {
        m_downMoveSpeed += m_frameTime * 1.0f;

        if (m_downMoveSpeed > (m_frameTime * 50.0f))
        {
            m_downMoveSpeed = m_frameTime * 50.0f;
        }
    }
    else
    {
        m_downMoveSpeed -= m_frameTime * 0.5f;

        if (m_downMoveSpeed < 0.0f)
        {
            m_downMoveSpeed = 0.0f;
        }
    }

    // Convert degrees to radians.
    radians = m_rotationY * 0.0174532925f;

    // Update the position.
    m_positionY -= cosf(radians) * m_downMoveSpeed;
    m_positionZ += sinf(radians) * m_downMoveSpeed;

    return;
}
