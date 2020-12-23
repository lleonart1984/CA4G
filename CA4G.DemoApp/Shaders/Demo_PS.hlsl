Texture2D<float4> tex : register(t0);

sampler smp : register(s0);

float4 main(float4 pos : SV_POSITION) : SV_TARGET
{
	return tex.Sample(smp, pos.xy);
}