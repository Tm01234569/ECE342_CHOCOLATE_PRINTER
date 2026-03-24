from PIL import Image

TARGET_WIDTH = 50
TARGET_HEIGHT = 50

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

with open("../Core/Inc/image_data.h", "w") as f:
    f.write("#ifndef IMAGE_DATA_H\n")
    f.write("#define IMAGE_DATA_H\n\n")
    f.write("#include <stdint.h>\n\n")
    f.write(f"#define IMAGE_WIDTH {width}\n")
    f.write(f"#define IMAGE_HEIGHT {height}\n\n")
    f.write(f"const uint8_t image_data[IMAGE_HEIGHT][IMAGE_WIDTH] = {{\n")

    for row in binary_array:
        row_text = ",".join(str(pixel) for pixel in row)
        f.write(f"    {{{row_text}}},\n")

    f.write("};\n\n")
    f.write("#endif\n")