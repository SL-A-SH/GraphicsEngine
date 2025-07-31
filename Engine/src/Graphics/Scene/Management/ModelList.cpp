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
    
    LOG("ModelList::Initialize - Creating " + std::to_string(numModels) + " models in ultra-dense multi-plane battle formation");
    LOG("ModelList::Initialize - Formation: 5 battle groups, 5 layers per group, ultra-dense spiral distribution");

    // Resize the vector to hold the model information.
    m_ModelInfoList.resize(m_modelCount);

    // Seed the random generator with the current time.
    srand((unsigned int)time(NULL));

    // Define ultra-dense multi-plane battle formation parameters for 10,000 ships
    float baseFormationRadius = 50.0f;    // Much closer base radius for dense formation
    float maxFormationRadius = 800.0f;    // Reduced max radius for closer ships
    float layerSpacing = 30.0f;           // Much tighter layer spacing
    float heightVariation = 400.0f;       // Reduced height variation for denser formation
    XMFLOAT3 targetPosition = XMFLOAT3(0.0f, 0.0f, 100.0f); // Common target point
    
    // Create a complex multi-plane formation with multiple battle groups
    for (i = 0; i < m_modelCount; i++)
    {
        // Create multiple battle groups and layers for better distribution
        int battleGroup = i / 5000;        // 5 battle groups of 5000 ships each
        int shipInGroup = i % 5000;
        int layer = shipInGroup / 1000;    // 5 layers per battle group
        int shipInLayer = shipInGroup % 1000;
        
        // Calculate base position for this battle group
        float groupAngle = (2.0f * 3.14159f * battleGroup) / 5.0f; // 5 groups in a circle
        float groupRadius = 100.0f + (battleGroup * 50.0f); // Much closer group distances
        
        // Calculate radius within the layer with more variation
        float layerRadius = baseFormationRadius + (layer * layerSpacing);
        if (layerRadius > maxFormationRadius) layerRadius = maxFormationRadius;
        
        // Calculate position in a dense spiral formation within the layer
        float angle = (2.0f * 3.14159f * shipInLayer) / 1000.0f;
        float radius = layerRadius + (rand() % 100 - 50); // Reduced variation for tighter formation
        
        // Add dense spiral variation to pack ships closer
        float spiralOffset = (shipInLayer * 0.05f) * (layer + 1); // Reduced spiral offset
        radius += spiralOffset;
        
        // Calculate final position with group offset
        float shipAngle = angle + groupAngle;
        float shipRadius = radius + groupRadius;
        
        m_ModelInfoList[i].positionX = cos(shipAngle) * shipRadius;
        m_ModelInfoList[i].positionY = (rand() % (int)heightVariation - heightVariation/2) * 0.1f + (layer * 20.0f);  // Tighter height planes
        m_ModelInfoList[i].positionZ = sin(shipAngle) * shipRadius;
        
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

    LOG("ModelList::Initialize - Ultra-dense multi-plane formation created successfully");
    LOG("ModelList::Initialize - Ships packed tightly across multiple height planes and battle groups");
    LOG("ModelList::Initialize - This dense formation will create massive FPS drops in CPU mode vs GPU mode");

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