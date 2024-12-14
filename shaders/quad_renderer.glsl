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


out vec4 fragmentColor;

// Calculate the distance from a point to a cube centered at `c` with size `s`
float distance_from_cube(in vec3 p, in vec3 c, float s) {
    vec3 d = abs(p - c) - vec3(s);
    return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

// Map function to define the scene
float map_the_world(in vec3 p) {
    // Cube centered at (0.0, 0.0, 0.0) with size 1.0
    float cube_0 = distance_from_cube(p, vec3(0.0), 1.0);

    // Add more objects here later...

    return cube_0;
}

// Perform ray marching to find intersections with the scene
vec3 ray_march(in vec3 ro, in vec3 rd) {
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 32;
    const float MINIMUM_HIT_DISTANCE = 0.001;
    const float MAXIMUM_TRACE_DISTANCE = 1000.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i) {
        vec3 current_position = ro + total_distance_traveled * rd;

        float distance_to_closest = map_the_world(current_position);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE) {
            return vec3(0.0, 1.0, 0.0);
        }

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE) {
            break;
        }
        total_distance_traveled += distance_to_closest;
    }
    return vec3(0.0); // Background color (black)
}

void main() {
    // Ray origin and direction
    vec3 ro = cameraPosition;
    vec3 rd = normalize(vec3(uv, 1.0)); // Ensure direction vector is normalized

    fragmentColor = vec4(ray_march(ro, rd), 1.0);
}
