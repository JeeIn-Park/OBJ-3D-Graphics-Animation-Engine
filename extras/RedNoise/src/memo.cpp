if (triangle.colour.name == "Water" || triangle.colour.name == "Glass") {
float refractiveIndex = 1.5; // Define the refractive index of the material

// Compute the surface normal and the incident ray direction
glm::vec3 normal = triangle.normal;
glm::vec3 incident = rayDirection;

// Check if the ray is entering or exiting the transparent material
float n1 = 1.0; // Refractive index of the medium outside the material
float n2 = refractiveIndex; // Refractive index of the material itself

if (glm::dot(normal, rayDirection) > 0) {
// Ray is inside the material
std::swap(n1, n2);
normal = -normal; // Reverse the normal direction
}

// Calculate the cosines of the angles between the rays and the normal
float cosTheta1 = glm::dot(-incident, normal);
float cosTheta2 = sqrt(1.0 - ((n1 / n2) * (n1 / n2)) * (1.0 - cosTheta1 * cosTheta1));

// Compute the direction of the transmitted (refracted) ray
glm::vec3 transmitted = (n1 / n2) * incident + ((n1 / n2) * cosTheta1 - cosTheta2) * normal;

// Trace the transmitted and reflected rays
RayTriangleIntersection transmittedIntersection = getClosestValidIntersection(intersection.intersectionPoint, transmitted, obj, false);
RayTriangleIntersection reflectedIntersection = getClosestValidIntersection(intersection.intersectionPoint, glm::reflect(incident, normal), obj, false);

// Calculate color contributions from transmitted and reflected rays
Colour transmittedColor = Colour(0, 0, 0);
Colour reflectedColor = Colour(0, 0, 0);

if (transmittedIntersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
// Compute color from transmitted ray (recursively if needed)
transmittedColor = ...; // Implement recursive calculation if needed
}

if (reflectedIntersection.distanceFromCamera < std::numeric_limits<float>::infinity()) {
// Compute color from reflected ray (recursively if needed)
reflectedColor = ...; // Implement recursive calculation if needed
}

// Calculate the blended color based on material properties
float blendFactor = 0.8; // Adjust the blending factor
Colour blendedColor = blendFactor * transmittedColor + (1 - blendFactor) * reflectedColor;

window.setPixelColour(x, y, blendedColor);
}
