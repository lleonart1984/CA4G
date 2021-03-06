/// Common header to any library that will use the scene retained for an accumulative path-tracing.

#include "CommonRT.h" // adds all 

// CB used to count passes of the PT and to know if the output image is a complexity image or the final color.
cbuffer AccumulativeInfo : register(b1, space1) {
	uint NumberOfPasses;
	uint ShowComplexity;
	float PathtracingRatio;
}

RWTexture2D<float3> Accumulation : register(u1, space1); // Auxiliar Accumulation Buffer
RWTexture2D<uint> Complexity	 : register(u2, space1); // Complexity buffer for debuging

#include "CommonComplexity.h"

void AccumulateOutput(uint2 coord, float3 value, int complexity) {
	Accumulation[coord] += value;
	Complexity[coord] += complexity;

	if (ShowComplexity) 
		Output[coord] = GetColor((int)round(Complexity[coord] / (NumberOfPasses + 1)));
	else 
		Output[coord] = Accumulation[coord] / (NumberOfPasses + 1);
}