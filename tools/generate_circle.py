#!/usr/bin/env python3
"""
Generate C struct for circular motion coordinates.
Outputs x/y positions relative to center (0,0) using FLOATTOFIX() macros.
"""

import math
import sys
import argparse


def generate_circle_coordinates(radius, resolution):
    """
    Generate circle coordinates with given radius and resolution.

    Args:
        radius: Circle radius in pixels
        resolution: Number of points around the circle

    Returns:
        List of (x, y) tuples representing circle coordinates
    """
    coordinates = []

    for i in range(resolution):
        angle = (2 * math.pi * i) / resolution
        x = radius * math.cos(angle)
        y = radius * math.sin(angle)
        coordinates.append((x, y))

    return coordinates


def generate_c_struct(radius, resolution, struct_name="circleCoords"):
    """
    Generate C struct definition with circle coordinates.

    Args:
        radius: Circle radius in pixels
        resolution: Number of points around the circle
        struct_name: Name of the C struct variable
    """
    coords = generate_circle_coordinates(radius, resolution)

    # Generate C code
    output = []
    output.append("// Generated circle coordinates")
    output.append(f"// Radius: {radius} pixels, Resolution: {resolution} points")
    output.append("")
    output.append("struct CircleCoord {")
    output.append("    WORD x;")
    output.append("    WORD y;")
    output.append("};")
    output.append("")
    output.append(f"static const struct CircleCoord {struct_name}[{resolution}] = {{")

    for i, (x, y) in enumerate(coords):
        comma = "," if i < len(coords) - 1 else ""
        output.append(f"    {{ FLOATTOFIX({x:.6f}), FLOATTOFIX({y:.6f}) }}{comma}")

    output.append("};")
    output.append("")
    output.append(f"#define CIRCLE_COORDS_COUNT {resolution}")

    return "\n".join(output)


def main():
    parser = argparse.ArgumentParser(
        description="Generate C struct for circular motion coordinates"
    )
    parser.add_argument(
        "radius",
        type=float,
        help="Circle radius in pixels"
    )
    parser.add_argument(
        "resolution",
        type=int,
        help="Number of points around the circle"
    )
    parser.add_argument(
        "-n", "--name",
        type=str,
        default="circleCoords",
        help="Name of the C struct variable (default: circleCoords)"
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        help="Output file (default: stdout)"
    )

    args = parser.parse_args()

    if args.resolution < 2:
        print("Error: Resolution must be at least 2", file=sys.stderr)
        sys.exit(1)

    if args.radius <= 0:
        print("Error: Radius must be positive", file=sys.stderr)
        sys.exit(1)

    # Generate the C struct
    c_code = generate_c_struct(args.radius, args.resolution, args.name)

    # Output to file or stdout
    if args.output:
        with open(args.output, 'w') as f:
            f.write(c_code)
        print(f"Generated circle coordinates written to {args.output}", file=sys.stderr)
    else:
        print(c_code)


if __name__ == "__main__":
    main()
