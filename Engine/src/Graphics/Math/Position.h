#ifndef _POSITION_H_
#define _POSITION_H_

#include <math.h>

class Position
{
public:
    Position();
    Position(const Position&);
    ~Position();

    void SetFrameTime(float);
    void GetRotation(float&);
    void GetRotationX(float&);
    void GetPosition(float&, float&, float&);

    void TurnLeft(bool);
    void TurnRight(bool);
    void LookUp(bool);
    void LookDown(bool);
    void MoveForward(bool);
    void MoveBackward(bool);
    void MoveLeft(bool);
    void MoveRight(bool);
    void MoveUp(bool);
    void MoveDown(bool);

private:
    float m_frameTime;
    float m_rotationY;
    float m_rotationX;
    float m_positionX, m_positionY, m_positionZ;
    float m_leftTurnSpeed, m_rightTurnSpeed;
    float m_upTurnSpeed, m_downTurnSpeed;
    float m_forwardMoveSpeed, m_backwardMoveSpeed;
    float m_leftMoveSpeed, m_rightMoveSpeed;
    float m_upMoveSpeed, m_downMoveSpeed;
};

#endif