#include "modellist.h"
#include <string>
#include "../../Core/System/Logger.h"


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

    // Go through all the models and randomly generate the position.
    for (i = 0; i < m_modelCount; i++)
    {
        // Generate positions in a larger grid pattern
        float gridSize = 50.0f;  // Size of the grid
        float spacing = 100.0f;   // Space between models
        
        // Calculate grid position
        int row = i % 3;         // 3 models per row
        int col = i / 3;         // Number of rows
        
        // Base position with some random offset
        m_ModelInfoList[i].positionX = (row * spacing) + (((float)rand() / RAND_MAX) * 5.0f - 2.5f);
        m_ModelInfoList[i].positionY = 0.0f;  // Keep models on the ground
        m_ModelInfoList[i].positionZ = (col * spacing) + (((float)rand() / RAND_MAX) * 5.0f - 2.5f);
        
        // Initialize rotation with random values for testing
        m_ModelInfoList[i].rotationX = ((float)rand() / RAND_MAX) * 0.5f - 0.25f;  // Small random rotation
        m_ModelInfoList[i].rotationY = ((float)rand() / RAND_MAX) * 6.28f;         // Full 360 degree rotation
        m_ModelInfoList[i].rotationZ = ((float)rand() / RAND_MAX) * 0.5f - 0.25f;  // Small random rotation
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