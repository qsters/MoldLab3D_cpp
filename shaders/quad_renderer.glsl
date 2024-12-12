#type vertex
#version 430 core

uniform vec2 screenSize;
in vec3 position;

out vec2 uv;


void main() {
    gl_Position = vec4(position, 1.0); // Use quad's position directly
    uv = position.xy; // Normalize from [-1, 1] to [0, 1]
}

#type fragment
#version 430 core

in vec2 uv;
out vec4 fragmentColor;

float distance_from_sphere(in vec3 p, in vec3 c, float r)
{
    return length(p - c) - r;
}

vec3 ray_march(in vec3 ro, in vec3 rd)
{
    float total_distance_traveled = 0.0;
    const int NUMBER_OF_STEPS = 32;
    const float MINIMUM_HIT_DISTANCE = 0.001;
    const float MAXIMUM_TRACE_DISTANCE = 1000.0;

    for (int i = 0; i < NUMBER_OF_STEPS; ++i)
    {
        vec3 current_position = ro + total_distance_traveled * rd;

        float distance_to_closest = distance_from_sphere(current_position, vec3(0.0), 1.0);

        if (distance_to_closest < MINIMUM_HIT_DISTANCE)
        {
            return vec3(1.0, 0.0, 0.0);
        }

        if (total_distance_traveled > MAXIMUM_TRACE_DISTANCE)
        {
            break;
        }
        total_distance_traveled += distance_to_closest;
    }
    return vec3(0.0);
}

void main() {
    vec3 camera_position = vec3(0.0, 0.0, -5.0);

    vec3 ro = camera_position;
    vec3 rd = vec3(uv, 1.0);

    fragmentColor = vec4(ray_march(ro, rd), 1.0); // Gradient color based on UV
}
