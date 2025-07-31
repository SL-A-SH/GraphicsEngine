// Stream Compaction Compute Shader
// TRUE GPU-DRIVEN RENDERING: Compacts visible objects into dense array
// Takes sparse visibility buffer and creates compacted array of visible object indices

// Constant buffer for object count
cbuffer ObjectCountBuffer : register(b0)
{
    uint objectCount;
    uint padding[3];
};

// Input visibility buffer (1 = visible, 0 = culled)
StructuredBuffer<uint> visibilityBuffer : register(t0);

// Output compacted visible objects array
RWStructuredBuffer<uint> visibleObjectsBuffer : register(u0);

// Atomic counter for visible object count
RWStructuredBuffer<uint> visibleCountBuffer : register(u1);

[numthreads(64, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    uint objectIndex = dispatchId.x;
    
    // Skip invalid objects
    if (objectIndex >= objectCount)
        return;
    
    // Check if this object is visible
    uint isVisible = visibilityBuffer[objectIndex];
    
    if (isVisible == 1)
    {
        // Atomically increment the visible count and get the insertion index
        uint insertionIndex;
        InterlockedAdd(visibleCountBuffer[0], 1, insertionIndex);
        
        // Store the visible object index in the compacted array
        visibleObjectsBuffer[insertionIndex] = objectIndex;
    }
}