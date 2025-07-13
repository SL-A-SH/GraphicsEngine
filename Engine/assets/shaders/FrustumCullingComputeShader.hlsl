// Frustum Culling Compute Shader
// Performs GPU-based frustum culling and generates draw commands

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

// Output draw command structure
struct DrawCommand
{
    uint indexCountPerInstance;
    uint instanceCount;
    uint startIndexLocation;
    int baseVertexLocation;
    uint startInstanceLocation;
};

// Frustum planes (6 planes: near, far, left, right, top, bottom)
cbuffer FrustumBuffer : register(b0)
{
    float4 frustumPlanes[6];
    float4 cameraPosition;
    float4 viewProjectionMatrix[4];
};

// Input buffers
StructuredBuffer<ObjectData> objectBuffer : register(t0);

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
    
    // If object is visible, generate draw command
    if (isVisible)
    {
        // Atomic increment to get unique index for visible objects
        uint visibleIndex;
        InterlockedAdd(visibleObjectCount[0], 1, visibleIndex);
        
        // Create draw command
        DrawCommand cmd;
        cmd.indexCountPerInstance = 36; // Assuming cube geometry (12 triangles * 3 indices)
        cmd.instanceCount = 1;
        cmd.startIndexLocation = 0;
        cmd.baseVertexLocation = 0;
        cmd.startInstanceLocation = 0;
        
        // Store draw command
        drawCommandBuffer[visibleIndex] = cmd;
    }
} 