struct PS_INPUT
{
    float4 Position : SV_POSITION;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    return input.Position.z / input.Position.w;
}
