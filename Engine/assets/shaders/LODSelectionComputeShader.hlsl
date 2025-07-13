// LOD Selection Compute Shader
// Performs GPU-based LOD selection based on distance from camera

// Input object data structure
struct ObjectData
{
    float3 position;
    float3 scale;
    float3 rotation;
    float3 boundingBoxMin;
    float3 boundingBoxMax;
    uint objectIndex;
    uint padding[2];
};

// LOD levels structure
struct LODLevel
{
    float distanceThreshold;
    uint indexCount;
    uint vertexOffset;
    uint indexOffset;
    uint padding;
};

// Output draw command structure
struct DrawCommand
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

// Camera and LOD data
cbuffer LODBuffer : register(b0)
{
    float4 cameraPosition;
    float4 cameraForward;
    float4 lodDistances[4]; // Distance thresholds for each LOD level
    uint maxLODLevels;
    uint padding[3];
};

// Input buffers
StructuredBuffer<ObjectData> objectBuffer : register(t0);
StructuredBuffer<LODLevel> lodLevelsBuffer : register(t1);

// Output buffers
RWStructuredBuffer<DrawCommand> drawCommandBuffer : register(u0);
RWStructuredBuffer<uint> visibleObjectCount : register(u1);

// Thread group size
[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Get object data
    ObjectData object = objectBuffer[objectIndex];
    
    // Calculate distance from camera to object
    float3 objectCenter = (object.boundingBoxMin + object.boundingBoxMax) * 0.5f * object.scale + object.position;
    float3 toCamera = cameraPosition.xyz - objectCenter;
    float distance = length(toCamera);
    
    // Determine LOD level based on distance
    uint selectedLOD = 0;
    for (uint i = 0; i < maxLODLevels; i++)
    {
        if (distance > lodDistances[i].x)
        {
            selectedLOD = i;
        }
        else
        {
            break;
        }
    }
    
    // Clamp to valid LOD range
    selectedLOD = min(selectedLOD, maxLODLevels - 1);
    
    // Get LOD level data
    LODLevel lodLevel = lodLevelsBuffer[selectedLOD];
    
    // Generate draw command for selected LOD
    uint visibleIndex;
    InterlockedAdd(visibleObjectCount[0], 1, visibleIndex);
    
    DrawCommand cmd;
    cmd.indexCountPerInstance = lodLevel.indexCount;
    cmd.instanceCount = 1;
    cmd.startIndexLocation = lodLevel.indexOffset;
    cmd.baseVertexLocation = lodLevel.vertexOffset;
    cmd.startInstanceLocation = 0;
    
    // Store draw command
    drawCommandBuffer[visibleIndex] = cmd;
} 