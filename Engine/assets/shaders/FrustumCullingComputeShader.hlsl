// Frustum Culling Compute Shader
// GPU-side frustum culling for object visibility determination

// Constant buffer for frustum planes and object count
cbuffer FrustumBuffer : register(b0)
{
    float4 frustumPlanes[6];  // Left, Right, Top, Bottom, Near, Far planes (normal.xyz, distance.w)
    uint objectCount;
    uint padding[3];
};

// Input object data structure (matches ObjectData from C++)
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

// Input buffers
StructuredBuffer<ObjectData> objectBuffer : register(t0);

// Output visibility buffer (1 = visible, 0 = culled)
RWStructuredBuffer<uint> visibilityBuffer : register(u0);

// Helper function to test AABB against frustum plane
bool TestAABBAgainstPlane(float3 aabbMin, float3 aabbMax, float4 plane)
{
    // Get the point on the AABB that is farthest along the plane normal
    float3 positiveVertex;
    positiveVertex.x = (plane.x >= 0.0f) ? aabbMax.x : aabbMin.x;
    positiveVertex.y = (plane.y >= 0.0f) ? aabbMax.y : aabbMin.y;
    positiveVertex.z = (plane.z >= 0.0f) ? aabbMax.z : aabbMin.z;
    
    // Test if the positive vertex is on the positive side of the plane
    float distance = dot(positiveVertex, plane.xyz) + plane.w;
    return distance >= 0.0f;
}

// Helper function to test AABB against all frustum planes
bool IsAABBVisible(float3 aabbMin, float3 aabbMax)
{
    // Test against all 6 frustum planes
    for (uint i = 0; i < 6; ++i)
    {
        if (!TestAABBAgainstPlane(aabbMin, aabbMax, frustumPlanes[i]))
        {
            return false; // Object is outside this plane, so it's culled
        }
    }
    return true; // Object passed all tests, so it's visible
}

[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Skip invalid objects
    if (objectIndex >= objectCount)
        return;
    
    // Get object data
    ObjectData object = objectBuffer[objectIndex];
    
    // Transform bounding box to world space
    float3 worldMin = object.boundingBoxMin * object.scale + object.position;
    float3 worldMax = object.boundingBoxMax * object.scale + object.position;
    
    // Perform frustum culling test
    bool isVisible = IsAABBVisible(worldMin, worldMax);
    
    // Store visibility result (1 = visible, 0 = culled)
    visibilityBuffer[objectIndex] = isVisible ? 1u : 0u;
} 