#ifndef _MODELLIST_H_
#define _MODELLIST_H_

#include <stdlib.h>
#include <time.h>
#include <vector>
#include <directxmath.h>

using namespace DirectX;

class ModelList
{
private:
    struct ModelInfoType
    {
        float positionX, positionY, positionZ;
        float rotationX, rotationY, rotationZ;
        float scaleX, scaleY, scaleZ;
    };

public:
    ModelList();
    ModelList(const ModelList&);
    ~ModelList();

    void Initialize(int);
    void Shutdown();

    int GetModelCount();
    void GetData(int, float&, float&, float&);
    void GetTransformData(int, float&, float&, float&, float&, float&, float&, float&, float&, float&);
    void SetTransformData(int, float, float, float, float, float, float, float, float, float);
    
    // Get all model instances for selection system
    const std::vector<ModelInfoType>& GetModelInstances() const { return m_ModelInfoList; }
    std::vector<ModelInfoType>& GetModelInstances() { return m_ModelInfoList; }

private:
    int m_modelCount;
    std::vector<ModelInfoType> m_ModelInfoList;
};

#endif