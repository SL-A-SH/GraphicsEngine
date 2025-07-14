// Command Generation Compute Shader
// Combines frustum culling and LOD selection to generate final draw commands

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

// Frustum and camera data
cbuffer CameraBuffer : register(b0)
{
    float4 frustumPlanes[6];
    float4 cameraPosition;
    float4 cameraForward;
    float4 lodDistances[4];
    uint maxLODLevels;
    uint objectCount;
    uint padding[2];
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
    
    // Check if this thread is processing a valid object
    if (objectIndex >= objectCount)
    {
        return; // Skip invalid objects
    }
    
    // Get object data
    ObjectData object = objectBuffer[objectIndex];
    
    // Transform bounding box to world space
    float3 worldMin = object.boundingBoxMin * object.scale + object.position;
    float3 worldMax = object.boundingBoxMax * object.scale + object.position;
    
    // Perform frustum culling
    bool isVisible = true;
    
    // Check against all 6 frustum planes
    for (int i = 0; i < 6; i++)
    {
        float4 plane = frustumPlanes[i];
        
        // Find the point on the AABB that is furthest in the negative direction of the plane normal
        float3 p;
        p.x = (plane.x >= 0.0f) ? worldMin.x : worldMax.x;
        p.y = (plane.y >= 0.0f) ? worldMin.y : worldMax.y;
        p.z = (plane.z >= 0.0f) ? worldMin.z : worldMax.z;
        
        // If the furthest point is behind the plane, the AABB is outside the frustum
        if (dot(plane.xyz, p) + plane.w < 0.0f)
        {
            isVisible = false;
            break;
        }
    }
    
    // If object is visible, perform LOD selection and generate draw command
    if (isVisible)
    {
        // Calculate distance from camera to object
        float3 objectCenter = (worldMin + worldMax) * 0.5f;
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
        
        // Atomic increment to get unique index for visible objects
        uint visibleIndex;
        InterlockedAdd(visibleObjectCount[0], 1, visibleIndex);
        
        // Create draw command
        DrawCommand cmd;
        cmd.indexCountPerInstance = lodLevel.indexCount;
        cmd.instanceCount = 1;
        cmd.startIndexLocation = lodLevel.indexOffset;
        cmd.baseVertexLocation = lodLevel.vertexOffset;
        cmd.startInstanceLocation = 0;
        
        // Store draw command
        drawCommandBuffer[visibleIndex] = cmd;
    }
} 