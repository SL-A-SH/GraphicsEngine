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
cbuffer FrustumBuffer : register(b0)
{
    float4 frustumPlanes[6];
    float4 cameraPosition;
    float4 cameraForward;
    float4 lodDistances[4];
    uint maxLODLevels;
    uint objectCount;
    uint currentPass; // 0 = visibility pass, 1 = command generation pass
    uint padding;
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
    
    // Debug: Check if object is at origin (which might indicate invalid data)
    if (objectIndex < 10) // Only check first 10 objects to avoid performance impact
    {
        // This will be visible in the debugger or GPU debugging tools
        // In a real implementation, you'd use a debug buffer to output this data
        // For now, we'll use the visibleObjectCount buffer to store debug info
        // Store object position in the debug buffer for first few objects
        if (objectIndex == 0)
        {
            // DISABLED: Prevent overwriting debug buffer from world matrix generation
            // Store debug info in the visibleObjectCount buffer
            // We'll read this back in the CPU code
            // visibleObjectCount[1] = asuint(object.position.x * 1000.0f); // Scale up for integer storage
            // visibleObjectCount[2] = asuint(object.position.y * 1000.0f);
            // visibleObjectCount[3] = asuint(object.position.z * 1000.0f);
            // visibleObjectCount[4] = asuint(worldMin.x * 1000.0f);
            // visibleObjectCount[5] = asuint(worldMin.y * 1000.0f);
            // visibleObjectCount[6] = asuint(worldMin.z * 1000.0f);
            // visibleObjectCount[7] = asuint(worldMax.x * 1000.0f);
            // visibleObjectCount[8] = asuint(worldMax.y * 1000.0f);
            // visibleObjectCount[9] = asuint(worldMax.z * 1000.0f);
        }
    }
    
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
    
    // If object is visible, increment the visible object count
    if (isVisible)
    {
        // Atomic increment to count visible objects
        InterlockedAdd(visibleObjectCount[0], 1);
    }
    
    // Only generate the draw command in the second pass (currentPass == 1)
    if (currentPass == 1 && objectIndex == 0)
    {
        // Get the total number of visible objects
        uint totalVisible = visibleObjectCount[0];
        
        // Create a single draw command for all visible instances
        DrawCommand cmd;
        cmd.indexCountPerInstance = 61260; // Fixed index count for the spaceship model
        cmd.instanceCount = totalVisible;
        cmd.startIndexLocation = 0;
        cmd.baseVertexLocation = 0;
        cmd.startInstanceLocation = 0;
        
        // Store the draw command at index 0
        drawCommandBuffer[0] = cmd;
    }
} 