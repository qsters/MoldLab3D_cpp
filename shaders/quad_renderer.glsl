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

vec3 gridMin = vec3(-1.0);                // Slightly below the grid's minimum
vec3 gridMax = vec3(gridSize) + vec3(1.0); // Slightly above the grid's maximum


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

// Map function to define the scene
float map_the_world(in vec3 point) {
    float result = 1e6; // Start with a very large value (infinite distance)
    float cubeSize = testValue; // Size of each cube

    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                // Calculate the grid position
                vec3 gridPoint = vec3(float(x), float(y), float(z));

                // Calculate the distance to the cube at this grid point
                float cube = distance_from_cube(point, gridPoint, cubeSize);

                // Combine distances using smooth_min for blending
                result = smooth_min(result, cube, 0.15);
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

// Perform ray marching to find intersections with the scene
vec3 ray_march(in vec3 rayOrigin, in vec3 rayDirection) {
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 50;
    const float MINIMUM_HIT_DISTANCE = 0.01;
    const float MAXIMUM_TRACE_DISTANCE = 50.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i) {
        vec3 current_position = rayOrigin + total_distance_traveled * rayDirection;

        float distance_to_closest = map_the_world(current_position);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) {
            return calculage_lighting(rayOrigin, current_position);
        }

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE) {
            return vec3(i / float(NUMBER_OF_STEPS), 0.0, 0.0);
        }
        total_distance_traveled += distance_to_closest;
    }
    return vec3(0.0); // Background color (black)
}

bool intersectsAABB(vec3 rayOrigin, vec3 rayDirection, vec3 gridMin, vec3 gridMax) {

    vec3 tMin = (gridMin - rayOrigin) / rayDirection;
    vec3 tMax = (gridMax - rayOrigin) / rayDirection;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
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

    // Cull rays that don't intersect the AABB
//    if (!intersectsAABB(rayOrigin, rayDirection, gridMin, gridMax)) {
//        fragmentColor = vec4(0.0, 0.0, 1.0, 0.0); // Background color
//        return;
//    }

    fragmentColor = vec4(ray_march(rayOrigin, rayDirection), 1.0);
}

