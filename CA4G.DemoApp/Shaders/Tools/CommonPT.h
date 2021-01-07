/// Common header to any library that will use the scene retained for an accumulative path-tracing.

#include "CommonRT.h" // adds all 

// CB used to count passes of the PT and to know if the output image is a complexity image or the final color.
cbuffer AccumulativeInfo : register(b1, space1) {
	uint NumberOfPasses;
	uint AccumulationIsComplexity;
}

RWTexture2D<float3> Accumulation : register(u1, space1); // Auxiliar Accumulation Buffer

#include "CommonComplexity.h"

void AccumulateOutput(uint2 coord, float3 value) {

	if (AccumulationIsComplexity) {

		float oldValue = Accumulation[coord].x;
		float currentValue = (value.x * 256 + value.y);
		//// Maximum complexity among frames
		//float newValue = max(oldValue, currentValue);

		// Average complexity among frames
		float newValue = (oldValue * NumberOfPasses + currentValue) / (NumberOfPasses + 1);

		Accumulation[coord] = float3(newValue, 0, 0);

		Output[coord] = GetColor((int)round(newValue));
	}
	else {
		Accumulation[coord] += value;
		Output[coord] = Accumulation[coord] / (NumberOfPasses + 1);
	}
}