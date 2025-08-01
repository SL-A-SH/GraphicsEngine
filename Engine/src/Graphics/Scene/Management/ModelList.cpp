#include "modellist.h"
#include <string>
#include "../../../Core/System/Logger.h"


ModelList::ModelList()
{
    m_modelCount = 0;
}


ModelList::ModelList(const ModelList& other)
{
}


ModelList::~ModelList()
{
}


void ModelList::Initialize(int numModels)
{
    int i;

    // Store the number of models.
    m_modelCount = numModels;
    



    m_ModelInfoList.resize(m_modelCount);

    // Seed the random generator with the current time.
    srand((unsigned int)time(NULL));

    float baseFormationRadius = 50.0f;
    float maxFormationRadius = 800.0f;
    float layerSpacing = 30.0f;
    float heightVariation = 400.0f;
    XMFLOAT3 targetPosition = XMFLOAT3(0.0f, 0.0f, 100.0f); // Common target point
    

    for (i = 0; i < m_modelCount; i++)
    {
        int battleGroup = i / 5000;
        int shipInGroup = i % 5000;
        int layer = shipInGroup / 1000;
        int shipInLayer = shipInGroup % 1000;
        
        float groupAngle = (2.0f * 3.14159f * battleGroup) / 5.0f;
        float groupRadius = 100.0f + (battleGroup * 50.0f);
        

        float layerRadius = baseFormationRadius + (layer * layerSpacing);
        if (layerRadius > maxFormationRadius) layerRadius = maxFormationRadius;
        

        float angle = (2.0f * 3.14159f * shipInLayer) / 1000.0f;
        float radius = layerRadius + (rand() % 100 - 50);
        
        float spiralOffset = (shipInLayer * 0.05f) * (layer + 1);
        radius += spiralOffset;
        

        float shipAngle = angle + groupAngle;
        float shipRadius = radius + groupRadius;
        
        m_ModelInfoList[i].positionX = cos(shipAngle) * shipRadius;
        m_ModelInfoList[i].positionY = (rand() % (int)heightVariation - heightVariation/2) * 0.1f + (layer * 20.0f);
        m_ModelInfoList[i].positionZ = sin(shipAngle) * shipRadius;
        

        XMFLOAT3 shipPos = XMFLOAT3(m_ModelInfoList[i].positionX, m_ModelInfoList[i].positionY, m_ModelInfoList[i].positionZ);
        XMFLOAT3 directionToTarget;
        directionToTarget.x = targetPosition.x - shipPos.x;
        directionToTarget.y = targetPosition.y - shipPos.y;
        directionToTarget.z = targetPosition.z - shipPos.z;
        

        float length = sqrt(directionToTarget.x * directionToTarget.x + 
                           directionToTarget.y * directionToTarget.y + 
                           directionToTarget.z * directionToTarget.z);
        directionToTarget.x /= length;
        directionToTarget.y /= length;
        directionToTarget.z /= length;
        

        float targetAngle = atan2(directionToTarget.x, directionToTarget.z);
        float randomVariation = ((float)rand() / RAND_MAX - 0.5f) * 0.2f; // ±0.1 radians
        
        m_ModelInfoList[i].rotationX = 1.570796f;
        m_ModelInfoList[i].rotationY = targetAngle + randomVariation;
        m_ModelInfoList[i].rotationZ = 0.0f;
        

        m_ModelInfoList[i].scaleX = 1.0f;
        m_ModelInfoList[i].scaleY = 1.0f;
        m_ModelInfoList[i].scaleZ = 1.0f;
    }



    return;
}


void ModelList::Shutdown()
{

    m_ModelInfoList.clear();
    m_modelCount = 0;
    return;
}


int ModelList::GetModelCount()
{
    return m_modelCount;
}


void ModelList::GetData(int index, float& positionX, float& positionY, float& positionZ)
{
    if (index >= 0 && index < m_modelCount)
    {
        positionX = m_ModelInfoList[index].positionX;
        positionY = m_ModelInfoList[index].positionY;
        positionZ = m_ModelInfoList[index].positionZ;
    }
    return;
}

void ModelList::GetTransformData(int index, float& positionX, float& positionY, float& positionZ, 
                                float& rotationX, float& rotationY, float& rotationZ,
                                float& scaleX, float& scaleY, float& scaleZ)
{
    if (index >= 0 && index < m_modelCount)
    {
        positionX = m_ModelInfoList[index].positionX;
        positionY = m_ModelInfoList[index].positionY;
        positionZ = m_ModelInfoList[index].positionZ;
        rotationX = m_ModelInfoList[index].rotationX;
        rotationY = m_ModelInfoList[index].rotationY;
        rotationZ = m_ModelInfoList[index].rotationZ;
        scaleX = m_ModelInfoList[index].scaleX;
        scaleY = m_ModelInfoList[index].scaleY;
        scaleZ = m_ModelInfoList[index].scaleZ;
    }
}

void ModelList::SetTransformData(int index, float positionX, float positionY, float positionZ,
                                float rotationX, float rotationY, float rotationZ,
                                float scaleX, float scaleY, float scaleZ)
{
    if (index >= 0 && index < m_modelCount)
    {
        m_ModelInfoList[index].positionX = positionX;
        m_ModelInfoList[index].positionY = positionY;
        m_ModelInfoList[index].positionZ = positionZ;
        m_ModelInfoList[index].rotationX = rotationX;
        m_ModelInfoList[index].rotationY = rotationY;
        m_ModelInfoList[index].rotationZ = rotationZ;
        m_ModelInfoList[index].scaleX = scaleX;
        m_ModelInfoList[index].scaleY = scaleY;
        m_ModelInfoList[index].scaleZ = scaleZ;
    }
}