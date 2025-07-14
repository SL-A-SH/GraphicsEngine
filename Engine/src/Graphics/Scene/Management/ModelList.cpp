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

    // Define battle formation parameters
    float formationRadius = 50.0f;  // Smaller radius for closer formation
    float spacing = 20.0f;         // Closer spacing between ships
    XMFLOAT3 targetPosition = XMFLOAT3(0.0f, 0.0f, 100.0f); // Common target point
    
    // Go through all the models and position them in a battle formation
    for (i = 0; i < m_modelCount; i++)
    {
        // Calculate position in a circular formation
        float angle = (2.0f * 3.14159f * i) / m_modelCount;
        float radius = formationRadius + (rand() % 20 - 10); // Slight random variation
        
        m_ModelInfoList[i].positionX = cos(angle) * radius;
        m_ModelInfoList[i].positionY = 0.0f;  // Keep all ships in the same plane
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
        // For a spaceship, we want to rotate around Y axis to face the target
        float targetAngle = atan2(directionToTarget.x, directionToTarget.z);
        
        // Add some random variation to make it look more natural
        float randomVariation = ((float)rand() / RAND_MAX - 0.5f) * 0.2f; // ±0.1 radians
        
        m_ModelInfoList[i].rotationX = 0.0f;  // No pitch for space battle
        m_ModelInfoList[i].rotationY = targetAngle + randomVariation;  // Face target with slight variation
        m_ModelInfoList[i].rotationZ = 0.0f;  // No roll for space battle
        
        // Scale all ships uniformly
        m_ModelInfoList[i].scaleX = 1.0f;
        m_ModelInfoList[i].scaleY = 1.0f;
        m_ModelInfoList[i].scaleZ = 1.0f;
        
        LOG("ModelList::Initialize - Model " + std::to_string(i) + " transform:");
        LOG("  Position: (" + std::to_string(m_ModelInfoList[i].positionX) + ", " + 
            std::to_string(m_ModelInfoList[i].positionY) + ", " + 
            std::to_string(m_ModelInfoList[i].positionZ) + ")");
        LOG("  Rotation: (" + std::to_string(m_ModelInfoList[i].rotationX) + ", " + 
            std::to_string(m_ModelInfoList[i].rotationY) + ", " + 
            std::to_string(m_ModelInfoList[i].rotationZ) + ")");
        LOG("  Scale: (" + std::to_string(m_ModelInfoList[i].scaleX) + ", " + 
            std::to_string(m_ModelInfoList[i].scaleY) + ", " + 
            std::to_string(m_ModelInfoList[i].scaleZ) + ")");
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