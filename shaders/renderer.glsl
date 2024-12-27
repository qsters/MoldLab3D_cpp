#type vertex
#version 430 core

uniform vec2 screenSize;

in vec3 position;

out vec2 uv;

void main() {
    gl_Position = vec4(position, 1.0); // Use quad's position directly
    uv = position.xy; // Pass normalized coordinates
}

#type fragment
#version 430 core

#define USE_TRANSPARENCY

in vec2 uv;

uniform float testValue;

out vec4 fragmentColor;

vec3 lightColor = vec3(1.0, 1.0, 1.0);    // Pure white light
vec3 objectColor = vec3(0.0, 1.0, 0.2);   // Reddish object

float maxCubeSideLength = 1.0;

#define SIMULATION_SETTINGS

// Define the voxel grid as a shader storage buffer
layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // Flattened 3D grid as a 1D array
};

layout(std430, binding = 2) buffer SettingsBuffer {
    SimulationData settings;
};

// After dispatching, buffer 4 is the data to read from for rendering
layout(rgba32f, binding = 0) uniform readonly image3D sdfData;


// Calculate the distance from a point to a cube centered at `c` with size `s`
float distance_from_cube(in vec3 point, in vec3 center, in float sideLength) {
    vec3 d = abs(point - center) - vec3(mix(0, maxCubeSideLength, sideLength) * 0.5f);
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float distance_from_sphere(in vec3 point, in vec3 center, in float radius) {
    // Distance from center minus the sphere radius
    return length(point - center) - radius;
}

float distance_from_rounded_cube(in vec3 point, in vec3 center, in float sideLength, float radius) {
    // Compute the half-size of the cube
    float halfSize = mix(0, maxCubeSideLength, sideLength) * 0.5;

    // Distance to the surface of the box minus the rounding radius
    vec3 d = abs(point - center) - vec3(halfSize - radius);
    return length(max(d, 0.0)) - radius;
}

float smooth_min(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * k * 0.25;
}

float smooth_max(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return max(a, b) + h * h * k * 0.25;
}

float map_the_world(in vec3 point) {
    float result = 1e6; // Start with a very large value (infinite distance)
    const int searchRadius = 1; // Local cube radius (adjustable)

    // Convert point to grid coordinates
    ivec3 center = ivec3(floor(point));

    int sdfReductionFactor = settings.sdf_reduction;

    ivec3 searchPoint = center / sdfReductionFactor;
    vec4 sdfValue = imageLoad(sdfData, searchPoint);

    float cameraSDF = distance_from_sphere(point, settings.camera_position, float(settings.grid_size) / 4.0);

    // skip this if the closest cube is less than the max betwen the search radius and the reduction factor times by the diagonal of the cube to make sure it will account for diagonal movement.
    if (sdfValue.w > max(sdfReductionFactor, searchRadius) * 1.8) {
        // subtract a bit off to make sure we do not overshoot
        result = sdfValue.w - sdfReductionFactor / 2.0;
        result = min(result, settings.grid_size / 2.0); // make sure it jumps no more than half the grid at one point to account for sdf values not set
        result = max(result, -cameraSDF);
        return result;
    }

    // Iterate only within a cube around the ray's current position
    for (int x = max(center.x - searchRadius, 0); x <= min(center.x + searchRadius, settings.grid_size - 1); x++) {
        for (int y = max(center.y - searchRadius, 0); y <= min(center.y + searchRadius, settings.grid_size - 1); y++) {
            for (int z = max(center.z - searchRadius, 0); z <= min(center.z + searchRadius, settings.grid_size - 1); z++) {
                int idx = x + settings.grid_size * (y + settings.grid_size * z); // Index for flattened 3D array

                // Skip zero-sized cubes
                if (voxelData[idx] <= 0.01) continue;

                // Calculate the grid position
                vec3 gridPoint = vec3(float(x), float(y), float(z));

                // Calculate the distance to the cube at this grid point
                float cube = distance_from_cube(point, gridPoint, voxelData[idx]);

                // Combine distances using smooth_min for blending
                result = smooth_min(result, cube, 0.55);
            }
        }
    }

    // Moves search radius if nothing has been encountered.
    result = min(searchRadius, result);
    result = max(result, -cameraSDF);
    return result; // Return the minimum distance for the scene
}

float map_the_world_transparent(in vec3 point) {
    float result = 1e6; // Start with a very large value (infinite distance)
    const int searchRadius = 1; // Local cube radius (adjustable)

    // Convert point to grid coordinates
    ivec3 center = ivec3(floor(point));

    int sdfReductionFactor = settings.sdf_reduction;

    ivec3 searchPoint = center / sdfReductionFactor;
    vec4 sdfValue = imageLoad(sdfData, searchPoint);

    float cameraSDF = distance_from_sphere(point, settings.camera_position, float(settings.grid_size) / 4.0);

    result = sdfValue.w;
    result = max(result, -cameraSDF);
    return result;

    // skip this if the closest cube is less than the max betwen the search radius and the reduction factor times by the diagonal of the cube to make sure it will account for diagonal movement.
    if (sdfValue.w > max(sdfReductionFactor, searchRadius) * 1.8) {
        // subtract a bit off to make sure we do not overshoot
        result = sdfValue.w - sdfReductionFactor / 2.0;
        result = min(result, settings.grid_size / 2.0); // make sure it jumps no more than half the grid at one point to account for sdf values not set
        result = max(result, -cameraSDF);
        return result;
    }

    // Iterate only within a cube around the ray's current position
    for (int x = max(center.x - searchRadius, 0); x <= min(center.x + searchRadius, settings.grid_size - 1); x++) {
        for (int y = max(center.y - searchRadius, 0); y <= min(center.y + searchRadius, settings.grid_size - 1); y++) {
            for (int z = max(center.z - searchRadius, 0); z <= min(center.z + searchRadius, settings.grid_size - 1); z++) {
                int idx = x + settings.grid_size * (y + settings.grid_size * z); // Index for flattened 3D array

                // Skip zero-sized cubes
                if (voxelData[idx] <= 0.01) continue;

                // Calculate the grid position
                vec3 gridPoint = vec3(float(x), float(y), float(z));

                // Calculate the distance to the cube at this grid point
                float cube = distance_from_cube(point, gridPoint, voxelData[idx]);

                // Combine distances using smooth_min for blending
                result = smooth_min(result, cube, 0.55);
            }
        }
    }

    // Moves search radius if nothing has been encountered.
    result = min(searchRadius, result);
    result = max(result, -cameraSDF);
    return result; // Return the minimum distance for the scene
}

// Calculate the normal at a point on the surface
vec3 calculate_normal(in vec3 point) {
    const float EPSILON = 0.01;
    float dx = map_the_world(point + vec3(EPSILON, 0.0, 0.0)) - map_the_world(point - vec3(EPSILON, 0.0, 0.0));
    float dy = map_the_world(point + vec3(0.0, EPSILON, 0.0)) - map_the_world(point - vec3(0.0, EPSILON, 0.0));
    float dz = map_the_world(point + vec3(0.0, 0.0, EPSILON)) - map_the_world(point - vec3(0.0, 0.0, EPSILON));
    return normalize(vec3(dx, dy, dz));
}

vec3 calculage_lighting(in vec3 rayOrigin, in vec3 current_position) {
    // Calculate normal at the hit point
    vec3 normal = calculate_normal(current_position);
    vec3 lightPosition = vec3(-5, settings.grid_size * 1.5f, -5); // Light above and slightly to the side

    // Calculate lighting
    vec3 lightDir = normalize(lightPosition - current_position); // Direction to light
    float diff = max(dot(normal, lightDir), 0.0); // Lambertian (diffuse) term

    // Calculate view direction
    vec3 viewDir = normalize(rayOrigin - current_position);

    // Combine light contributions
    vec3 ambient = 0.1 * lightColor; // Ambient lighting
    vec3 diffuse = diff * lightColor; // Diffuse lighting

    vec3 light = diffuse + ambient ; // Combine all light components

    vec3 gradient = current_position / vec3(settings.grid_size);

    return gradient; // Multiply by object color
}

// Perform ray marching to find intersections with the scene
vec3 ray_march(in vec3 rayOrigin, in vec3 rayDirection) {
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 500;
    const float MINIMUM_HIT_DISTANCE = 0.01;
    // Diagonal of a cube side length * sqrt(3)
    const float MAXIMUM_TRACE_DISTANCE = settings.grid_size * 1.732;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i) {
        vec3 current_position = rayOrigin + total_distance_traveled * rayDirection;

        // If traveled too far, or exited the bounds, return red (for now)
        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE || distance_from_cube(current_position, settings.camera_focus, settings.grid_size) > 1) {
            return vec3(i / float(NUMBER_OF_STEPS), 0.0, 0.0);
        }

        float distance_to_closest = map_the_world(current_position);
//        return vec3(distance_to_closest);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) {
            return calculage_lighting(rayOrigin, current_position);
        }

        total_distance_traveled += distance_to_closest;
    }
    return vec3(0.0); // Background color (black)
}

vec3 ray_march_transparency(in vec3 rayOrigin, in vec3 rayDirection) {
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 700;
    const float MINIMUM_HIT_DISTANCE = .1;
    const float STEP_MARCH_DISTANCE = 1;
    // Diagonal of a cube side length * sqrt(3)
    const float MAXIMUM_TRACE_DISTANCE = settings.grid_size * 1.732;

    float opacity_accumulator = 0.0;
    float opacity_amount_per_step = (2 / float(settings.grid_size)) * STEP_MARCH_DISTANCE;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i) {
        vec3 current_position = rayOrigin + total_distance_traveled * rayDirection;

        float traveled_this_step = 0.0;

        // If traveled too far, return red (for now)
        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE) {
            return vec3(i / float(NUMBER_OF_STEPS), 0.0, 0.0);
        }

        // If exited the bounds, or opacity is full, return opacity
        if (distance_from_cube(current_position, settings.camera_focus, settings.grid_size) > 1 || opacity_accumulator >= 1.0f) {
            return vec3(opacity_accumulator);
        }

        float distance_to_closest = map_the_world_transparent(current_position);
        traveled_this_step = distance_to_closest;
        //        return vec3(distance_to_closest);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) {
            ivec3 gridCoord = clamp(ivec3(floor(current_position)), ivec3(0), ivec3(settings.grid_size - 1)); // Convert to grid coordinates
            int voxelIndex = gridCoord.x + settings.grid_size * (gridCoord.y + settings.grid_size * gridCoord.z);

            opacity_accumulator += (10 * voxelData[voxelIndex]) / settings.grid_size;
            traveled_this_step = STEP_MARCH_DISTANCE;
        }

        total_distance_traveled += traveled_this_step;
    }
    return vec3(0.0, 0.0, 1.0); // Background color (black)
}

bool intersectsAABB(vec3 rayOrigin, vec3 rayDirection, vec3 gridMin, vec3 gridMax, out float tNear) {
    vec3 tMin = (gridMin - rayOrigin) / rayDirection;
    vec3 tMax = (gridMax - rayOrigin) / rayDirection;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tNear <= tFar && tFar >= 0.0;
}

void main() {
    // Calculate camera orientation
    vec3 forward = normalize(settings.camera_focus - settings.camera_position); // Forward direction
    vec3 worldUp = vec3(0.0, 1.0, 0.0); // World up vector
    vec3 right = normalize(cross(worldUp, forward)); // Right vector
    vec3 up = cross(forward, right); // Up vector

    // Ray origin and direction
    vec3 rayOrigin = settings.camera_position;
    vec3 rayDirection = normalize(uv.x * right + uv.y * up + forward); // Combine screen-space uv with camera orientation

    vec3 gridMin = vec3(0.0);
    vec3 gridMax = vec3(settings.grid_size - 1);
    vec3 offset = vec3(0.5); // offset to account for cube thickness

    float tNear;
    // Cull rays that don't intersect the AABB
    if (!intersectsAABB(rayOrigin, rayDirection, gridMin - offset, gridMax + offset, tNear)) {
        fragmentColor = vec4(0.0, 0.0, 0.0, 1.0); // Background color
        return;
    }

    // Advance the ray origin to the intersection point with the AABB
    rayOrigin += rayDirection * max(tNear - 0.001, 0.0); // Ensure tNear is non-negative

    // Perform ray marching from the AABB intersection point
    // Perform ray marching from the AABB intersection point
    #ifdef USE_TRANSPARENCY
    fragmentColor = vec4(ray_march_transparency(rayOrigin, rayDirection), 1.0);
    #else
    fragmentColor = vec4(ray_march(rayOrigin, rayDirection), 1.0);
    #endif

}

//void main() {
//    // Compute the reduced grid size
//    int reducedGridSize = settings.grid_size / sdfReductionFactor;
//
//    // Map screen-space UV to grid coordinates
//    ivec2 gridPos = ivec2(uv * reducedGridSize);
//
//    // Ensure the coordinates are within bounds
//    if (gridPos.x < 0 || gridPos.y < 0 || gridPos.x >= reducedGridSize || gridPos.y >= reducedGridSize) {
//        fragmentColor = vec4(0.0, 0.0, 0.0, 1.0); // Out of bounds, render black
//        return;
//    }
//
//    // Compute the 1D index for the slice at `sliceIndex`
//    int index = gridPos.x + reducedGridSize * (gridPos.y + reducedGridSize * 200);
//
//    // Read the SDF value
//    vec4 sdfEntry = sdfData[index];
//
//    // Map the distance value to a color
//    float distance = sdfEntry.w;
//    vec3 color;
//    if (distance < 1e6) {
//        // Normalize distance and map to color gradient
//        float normalizedDistance = clamp(distance / float(settings.grid_size), 0.0, 1.0);
//        color = vec3(1.0 - normalizedDistance, normalizedDistance, 0.0); // Gradient from red to green
//    } else {
//        color = vec3(0.0, 0.0, 0.0); // Empty cells are black
//    }
//
//    fragmentColor = vec4(color, 1.0);
//}
