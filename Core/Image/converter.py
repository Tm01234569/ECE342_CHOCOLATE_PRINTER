from PIL import Image

TARGET_WIDTH = 25
TARGET_HEIGHT = 25

img = Image.open("Image/image.png").convert("RGB")
img = img.resize((TARGET_WIDTH, TARGET_HEIGHT), Image.Resampling.NEAREST)

pixels = img.load()

width, height = img.size

binary_array = []

for y in range(height):
    row = []
    for x in range(width):
        r, g, b = pixels[x, y]

        brightness = 0.299 * r + 0.587 * g + 0.114 * b

        if brightness > 128:
            row.append(1)
        else:
            row.append(0)

    binary_array.append(row)

# ---------- RLE compression ----------
runs_per_row = []

for y in range(height):

    runs = []
    x = 0

    while x < width:

        # skip white pixels
        while x < width and binary_array[y][x] == 1:
            x += 1

        if x >= width:
            break

        start = x

        # count black pixels
        while x < width and binary_array[y][x] == 0:
            x += 1

        length = x - start

        runs.append((start, length))

    runs_per_row.append(runs)

# ---------- write header ----------
MAX_RUNS = max(len(r) for r in runs_per_row)

with open("../Core/Inc/image_data.h", "w") as f:

    f.write("#ifndef IMAGE_DATA_H\n")
    f.write("#define IMAGE_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")

    f.write(f"#define IMAGE_WIDTH {width}\n")
    f.write(f"#define IMAGE_HEIGHT {height}\n")
    f.write(f"#define MAX_RUNS {MAX_RUNS}\n\n")

    f.write("typedef struct {\n")
    f.write("    uint8_t start;\n")
    f.write("    uint8_t length;\n")
    f.write("} Run;\n\n")

    f.write(f"const Run image_runs[IMAGE_HEIGHT][MAX_RUNS] = {{\n")

    for row in runs_per_row:

        row_runs = row + [(0,0)]*(MAX_RUNS-len(row))

        f.write("    {")
        f.write(",".join(f"{{{s},{l}}}" for s,l in row_runs))
        f.write("},\n")

    f.write("};\n\n")

    f.write("const uint8_t run_counts[IMAGE_HEIGHT] = {")
    f.write(",".join(str(len(r)) for r in runs_per_row))
    f.write("};\n\n")

    f.write("#endif\n")