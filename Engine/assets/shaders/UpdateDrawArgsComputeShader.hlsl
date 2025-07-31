// Update Draw Arguments Compute Shader
// TRUE GPU-DRIVEN RENDERING: Updates DrawIndexedInstancedIndirect arguments
// Sets up the argument buffer for indirect drawing based on visible object count

// Constant buffer for static draw info
cbuffer DrawInfoBuffer : register(b0)
{
    uint indexCountPerInstance;  // Number of indices per model instance
    uint startIndexLocation;     // Starting index location
    int baseVertexLocation;      // Base vertex location
    uint startInstanceLocation;  // Starting instance location
};

// Input visible count buffer
StructuredBuffer<uint> visibleCountBuffer : register(t0);

// Output draw arguments buffer (5 UINTs for DrawIndexedInstancedIndirect)
RWStructuredBuffer<uint> drawArgumentsBuffer : register(u0);

[numthreads(1, 1, 1)]
void main(uint3 dispatchId : SV_DispatchThreadID)
{
    // Only need one thread to update the draw arguments
    if (dispatchId.x != 0)
        return;
    
    // Get the visible object count from stream compaction
    uint visibleInstanceCount = visibleCountBuffer[0];
    
    // Update DrawIndexedInstancedIndirect arguments
    // Structure: IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation
    drawArgumentsBuffer[0] = indexCountPerInstance;     // IndexCountPerInstance
    drawArgumentsBuffer[1] = visibleInstanceCount;      // InstanceCount (GPU-computed!)
    drawArgumentsBuffer[2] = startIndexLocation;        // StartIndexLocation
    drawArgumentsBuffer[3] = baseVertexLocation;        // BaseVertexLocation (as UINT)
    drawArgumentsBuffer[4] = startInstanceLocation;     // StartInstanceLocation
}