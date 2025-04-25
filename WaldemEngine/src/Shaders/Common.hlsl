float3 GetForwardVector(matrix world)
{
    float3 forward = transpose(world)[2].xyz;
    return normalize(forward);
}