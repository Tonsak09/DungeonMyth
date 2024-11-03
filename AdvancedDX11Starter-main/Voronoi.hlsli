/*
William Duprey
11/3/24
Voronoi Shader Header
*/

#ifndef __VORONOI__ 
#define __VORONOI__

////////////////////////////////////////////////////////////////////////////////
// --------------------------- HELPER FUNCTIONS ----------------------------- //
////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------
// Performs a bunch of arbitrary steps 
// to produce a deterministically random float2.
// --------------------------------------------------------
inline float2 random_float2(float2 uv, float offset)
{
    // 2x2 matrix of arbitrary values
    float2x2 m = float2x2(
        15.27f, 47.63f,
        89.98f, 95.07f
    );
    
    // Do some arbitrary, random steps
    uv = frac(sin(mul(uv, m)) * 46839.32f);

    // Make a new float2, scaled by angle offset
    return float2(
        sin(uv.x * offset) * 0.5f + 0.5f,
        cos(uv.y * offset) * 0.5f + 0.5f
    );
}

// --------------------------------------------------------
// Performs a bunch of arbitrary steps 
// to produce a deterministically random float3.
// --------------------------------------------------------
inline float3 random_float3(float3 pos, float offset)
{
    // 3x3 matrix of arbitrary values
    float3x3 m = float3x3(
        15.27f, 47.63f, 99.41f,
        89.98f, 95.07f, 38.39f,
        33.83f, 51.06f, 60.77f
    );
    
    // Do some arbitrary, random steps
    pos = frac(sin(mul(pos, m)) * 46839.32f);

    // Make a new float3, scaled by angle offset
    return float3(
        sin(pos.x * offset) * 0.5f + 0.5f,
        cos(pos.y * offset) * 0.5f + 0.5f,
        sin(pos.z * offset) * 0.5f + 0.5f
    );
}

////////////////////////////////////////////////////////////////////////////////
// ------------------------------- VORONOI ---------------------------------- //
////////////////////////////////////////////////////////////////////////////////
// --------------------------------------------------------
// Does some stuff that's still a little over my head in
// terms of the actual Voronoi algorithm, but the result 
// is a 2D pattern of cells whose density and angle offset
// can be easily changed, and is based on the input's uv.
// --------------------------------------------------------
inline float Voronoi2D(float2 uv, float angle, float density)
{
    // Tile the space based on the cell density
    float2 g = floor(uv * density);
    float2 f = frac(uv * density);

    // Track the closest distance, used to color the pixel
    float minDist = 1.0f;
    
    // Loop through each neighboring tile in 2 dimensions
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            // Make a float3 for the neighbor cell we're looking at
            float2 neighbor = float2(x, y);
                
            // Get its deterministically random position,
            // using an angle offset
            float2 offset = random_float2(g + neighbor, angle);
                
            // Vector to that neighbor
            float2 diff = neighbor + offset - f;

            // Magnitude of that vector
            float dist = length(diff);
                
            // Keep the closer distance
            minDist = min(minDist, dist);
        }
    }
    
    return minDist;
}

// --------------------------------------------------------
// Does some stuff that's still a little over my head in
// terms of the actual Voronoi algorithm, but the result 
// is a 3D pattern of cells whose density and angle offset
// can be easily changed, and is based on the input's 
// world position.
// --------------------------------------------------------
inline float Voronoi3D(float3 pos, float angle, float density)
{
    // Tile the space based on the cell density
    float3 g = floor(pos * density);
    float3 f = frac(pos * density);

    // Track the closest distance, used to color the pixel
    float minDist = 1.0f;
    
    // Loop through each neighboring tile in 3 dimensions
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int z = -1; z <= 1; z++)
            {
                // Make a float3 for the neighbor cell we're looking at
                float3 neighbor = float3(x, y, z);
                
                // Get its deterministically random position,
                // using an angle offset
                float3 offset = random_float3(g + neighbor, angle);
                
                // Vector to that neighbor
                float3 diff = neighbor + offset - f;

                // Magnitude of that vector
                float dist = length(diff);
                
                // Keep the closer distance
                minDist = min(minDist, dist);
            }
        }
    }
    
    return minDist;
}

#endif