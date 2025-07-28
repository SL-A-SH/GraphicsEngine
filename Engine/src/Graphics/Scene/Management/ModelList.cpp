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
    
    LOG("ModelList::Initialize - Creating " + std::to_string(numModels) + " models");

    // Resize the vector to hold the model information.
    m_ModelInfoList.resize(m_modelCount);

    // Seed the random generator with the current time.
    srand((unsigned int)time(NULL));

    // Define battle formation parameters for thousands of ships
    float baseFormationRadius = 400.0f;  // Spacing between ships
    float maxFormationRadius = 2000.0f;  // Radius for outer ships
    float layerSpacing = 200.0f;         // Spacing between layers
    XMFLOAT3 targetPosition = XMFLOAT3(0.0f, 0.0f, 100.0f); // Common target point
    
    // Go through all the models and position them in a multi-layered battle formation
    for (i = 0; i < m_modelCount; i++)
    {
        // Create multiple layers of formations with better distribution
        int layer = i / 1000;  // Each layer has ~1000 ships
        int shipInLayer = i % 1000;
        
        // Calculate radius based on layer with increased spacing
        float layerRadius = baseFormationRadius + (layer * layerSpacing);
        if (layerRadius > maxFormationRadius) layerRadius = maxFormationRadius;
        
        // Calculate position in a circular formation within the layer with more spacing
        float angle = (2.0f * 3.14159f * shipInLayer) / 1000.0f;
        float radius = layerRadius + (rand() % 200 - 100); // More variation for natural look
        
        m_ModelInfoList[i].positionX = cos(angle) * radius;
        m_ModelInfoList[i].positionY = (rand() % 400 - 200) * 0.1f;  // More height variation
        m_ModelInfoList[i].positionZ = sin(angle) * radius;
        
        // Calculate direction to target for proper orientation
        XMFLOAT3 shipPos = XMFLOAT3(m_ModelInfoList[i].positionX, m_ModelInfoList[i].positionY, m_ModelInfoList[i].positionZ);
        XMFLOAT3 directionToTarget;
        directionToTarget.x = targetPosition.x - shipPos.x;
        directionToTarget.y = targetPosition.y - shipPos.y;
        directionToTarget.z = targetPosition.z - shipPos.z;
        
        // Normalize the direction
        float length = sqrt(directionToTarget.x * directionToTarget.x + 
                           directionToTarget.y * directionToTarget.y + 
                           directionToTarget.z * directionToTarget.z);
        directionToTarget.x /= length;
        directionToTarget.y /= length;
        directionToTarget.z /= length;
        
        // Calculate rotation to face the target
        float targetAngle = atan2(directionToTarget.x, directionToTarget.z);
        float randomVariation = ((float)rand() / RAND_MAX - 0.5f) * 0.2f; // ±0.1 radians
        
        // Fix the model orientation: add 90 degrees (π/2 radians) around X-axis to make ships face forward
        m_ModelInfoList[i].rotationX = 1.570796f;  // 90 degrees in radians (π/2)
        m_ModelInfoList[i].rotationY = targetAngle + randomVariation;  // Face target with slight variation
        m_ModelInfoList[i].rotationZ = 0.0f;  // No roll for space battle
        
        // Scale all ships uniformly
        m_ModelInfoList[i].scaleX = 1.0f;
        m_ModelInfoList[i].scaleY = 1.0f;
        m_ModelInfoList[i].scaleZ = 1.0f;
    }

    return;
}


void ModelList::Shutdown()
{
    // Clear the model information list.
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