#include "modellist.h"


ModelList::ModelList()
{
    m_ModelInfoList = 0;
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

    // Create a list array of the model information.
    m_ModelInfoList = new ModelInfoType[m_modelCount];

    // Seed the random generator with the current time.
    srand((unsigned int)time(NULL));

    // Go through all the models and randomly generate the position.
    for (i = 0; i < m_modelCount; i++)
    {
        // Generate positions in a larger grid pattern
        float gridSize = 50.0f;  // Size of the grid
        float spacing = 100.0f;   // Space between models
        
        // Calculate grid position
        int row = i % 3;         // 5 models per row
        int col = i / 3;         // Number of rows
        
        // Base position with some random offset
        m_ModelInfoList[i].positionX = (row * spacing) + (((float)rand() / RAND_MAX) * 5.0f - 2.5f);
        m_ModelInfoList[i].positionY = 0.0f;  // Keep models on the ground
        m_ModelInfoList[i].positionZ = (col * spacing) + (((float)rand() / RAND_MAX) * 5.0f - 2.5f);
    }

    return;
}


void ModelList::Shutdown()
{
    // Release the model information list.
    if (m_ModelInfoList)
    {
        delete[] m_ModelInfoList;
        m_ModelInfoList = 0;
    }

    return;
}


int ModelList::GetModelCount()
{
    return m_modelCount;
}


void ModelList::GetData(int index, float& positionX, float& positionY, float& positionZ)
{
    positionX = m_ModelInfoList[index].positionX;
    positionY = m_ModelInfoList[index].positionY;
    positionZ = m_ModelInfoList[index].positionZ;
    return;
}