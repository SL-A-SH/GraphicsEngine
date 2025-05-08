#pragma once

#include <DirectXMath.h>

namespace Engine {

    class Transform {
    public:
        Transform();
        ~Transform();

        // Position
        void SetPosition(const DirectX::XMFLOAT3& position);
        void SetPosition(float x, float y, float z);
        const DirectX::XMFLOAT3& GetPosition() const { return m_position; }
        void Translate(const DirectX::XMFLOAT3& translation);
        void Translate(float x, float y, float z);

        // Rotation
        void SetRotation(const DirectX::XMFLOAT4& quaternion);
        void SetRotation(float pitch, float yaw, float roll);
        const DirectX::XMFLOAT4& GetRotation() const { return m_rotation; }
        void Rotate(const DirectX::XMFLOAT4& quaternion);
        void Rotate(float pitch, float yaw, float roll);

        // Scale
        void SetScale(const DirectX::XMFLOAT3& scale);
        void SetScale(float x, float y, float z);
        void SetScale(float uniformScale);
        const DirectX::XMFLOAT3& GetScale() const { return m_scale; }
        void Scale(const DirectX::XMFLOAT3& scale);
        void Scale(float x, float y, float z);
        void Scale(float uniformScale);

        // Transformation matrices
        DirectX::XMMATRIX GetWorldMatrix() const;
        DirectX::XMMATRIX GetViewMatrix() const;
        DirectX::XMMATRIX GetProjectionMatrix() const;

        // Helper methods
        DirectX::XMFLOAT3 TransformPoint(const DirectX::XMFLOAT3& point) const;
        DirectX::XMFLOAT3 TransformDirection(const DirectX::XMFLOAT3& direction) const;
        DirectX::XMFLOAT3 GetForward() const;
        DirectX::XMFLOAT3 GetRight() const;
        DirectX::XMFLOAT3 GetUp() const;

    private:
        DirectX::XMFLOAT3 m_position;
        DirectX::XMFLOAT4 m_rotation; // Quaternion
        DirectX::XMFLOAT3 m_scale;

        mutable DirectX::XMMATRIX m_worldMatrix;
        mutable bool m_worldMatrixDirty;
    };

} // namespace Engine