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

in vec2 uv;
uniform vec3 cameraPosition;
uniform vec3 focusPoint;
uniform int gridSize;

uniform float testValue;

out vec4 fragmentColor;

vec3 lightPosition = vec3(-5, gridSize * 1.5f, -5); // Light above and slightly to the side
vec3 lightColor = vec3(1.0, 1.0, 1.0);    // Pure white light
vec3 objectColor = vec3(0.0, 1.0, 0.2);   // Reddish object

float maxCubeSideLength = 0.9;

vec3 gridMin = vec3(0.0);                // Slightly below the grid's minimum
vec3 gridMax = vec3(gridSize - 1); // Slightly above the grid's maximum

vec3 AABB_offset = vec3(0.51);

// Define the voxel grid as a shader storage buffer
layout(std430, binding = 0) buffer VoxelGrid {
    float voxelData[]; // Flattened 3D grid as a 1D array
};

// Calculate the distance from a point to a cube centered at `c` with size `s`
float distance_from_cube(in vec3 point, in vec3 center, in float sideLength) {
    vec3 d = abs(point - center) - vec3(mix(0, maxCubeSideLength, sideLength) * 0.5f);
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
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
    const int searchRadius = 2; // Local cube radius (adjustable)

    // Convert point to grid coordinates
    ivec3 center = ivec3(floor(point / testValue));

    // Iterate only within a cube around the ray's current position
    for (int x = max(center.x - searchRadius, 0); x <= min(center.x + searchRadius, gridSize - 1); x++) {
        for (int y = max(center.y - searchRadius, 0); y <= min(center.y + searchRadius, gridSize - 1); y++) {
            for (int z = max(center.z - searchRadius, 0); z <= min(center.z + searchRadius, gridSize - 1); z++) {
                int idx = x + gridSize * (y + gridSize * z); // Index for flattened 3D array

                // Skip zero-sized cubes
                if (voxelData[idx] <= 0.01) continue;

                // Calculate the grid position
                vec3 gridPoint = vec3(float(x), float(y), float(z)) * testValue;

                // Calculate the distance to the cube at this grid point
                float cube = distance_from_cube(point, gridPoint, voxelData[idx]);

                // Combine distances using smooth_min for blending
                result = smooth_min(result, cube, .75);
            }
        }
    }
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

    // Calculate lighting
    vec3 lightDir = normalize(lightPosition - current_position); // Direction to light
    float diff = max(dot(normal, lightDir), 0.0); // Lambertian (diffuse) term

    // Calculate view direction
    vec3 viewDir = normalize(rayOrigin - current_position);

    // Combine light contributions
    vec3 ambient = 0.1 * lightColor; // Ambient lighting
    vec3 diffuse = diff * lightColor; // Diffuse lighting

    vec3 color = diffuse + ambient ; // Combine all light components

    return color * objectColor; // Multiply by object color
}

bool outside_AABB(in vec3 position) {
    float distance_to_grid = distance_from_cube(position, focusPoint, gridSize + AABB_offset.x * 5.0);
    return distance_to_grid > 0.001;
}

// Perform ray marching to find intersections with the scene
vec3 ray_march(in vec3 rayOrigin, in vec3 rayDirection) {
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 150;
    const float MINIMUM_HIT_DISTANCE = 0.01;
    const float MAXIMUM_TRACE_DISTANCE = 50.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i) {
        vec3 current_position = rayOrigin + total_distance_traveled * rayDirection;

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE) {
            return vec3(1 - (i / float(NUMBER_OF_STEPS)), 0.0, 0.0);
        }

        float distance_to_closest = map_the_world(current_position);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) {
            return calculage_lighting(rayOrigin, current_position);
        }

        total_distance_traveled += distance_to_closest;
    }
    return vec3(0.0); // Background color (black)
}

bool intersectsAABB(vec3 rayOrigin, vec3 rayDirection, vec3 gridMin, vec3 gridMax, out float tNear) {
    vec3 adjustedMin = gridMin - AABB_offset;
    vec3 adjustedMax = gridMax + AABB_offset;

    vec3 tMin = (adjustedMin - rayOrigin) / rayDirection;
    vec3 tMax = (adjustedMax - rayOrigin) / rayDirection;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tNear <= tFar && tFar >= 0.0;
}

void main() {
    // Calculate camera orientation
    vec3 forward = normalize(focusPoint - cameraPosition); // Forward direction
    vec3 worldUp = vec3(0.0, 1.0, 0.0); // World up vector
    vec3 right = normalize(cross(worldUp, forward)); // Right vector
    vec3 up = cross(forward, right); // Up vector

    // Ray origin and direction
    vec3 rayOrigin = cameraPosition;
    vec3 rayDirection = normalize(uv.x * right + uv.y * up + forward); // Combine screen-space uv with camera orientation

    float tNear;
    // Cull rays that don't intersect the AABB
    if (!intersectsAABB(rayOrigin, rayDirection, gridMin, gridMax, tNear)) {
        fragmentColor = vec4(0.0, 0.0, 1.0, 1.0); // Background color
        return;
    }
    // Advance the ray origin to the intersection point with the AABB
    rayOrigin += rayDirection * max(tNear - 0.001, 0.0); // Ensure tNear is non-negative



    // Perform ray marching from the AABB intersection point
    fragmentColor = vec4(ray_march(rayOrigin, rayDirection), 1.0);
}

