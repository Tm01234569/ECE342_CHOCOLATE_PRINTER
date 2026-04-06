/*
 * Image loading and resizing uses the stb libraries:
 *
 * stb_image.h
 * stb_image_resize2.h
 *
 * Author: Sean Barrett
 * Source: https://github.com/nothings/stb
 * 
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#define TARGET_WIDTH 25
#define TARGET_HEIGHT 25

typedef struct {
  uint8_t start;
  uint8_t length;
} Run;

int main(void) {
  int original_w, original_h, channels;

  // Load image as RGB
  unsigned char* img = stbi_load("image.png", &original_w, &original_h, &channels, 3);
  if (!img) {
    printf("Failed to load image\n");
    return 1;
  }

  // Resize image
  unsigned char* resized = malloc(TARGET_WIDTH * TARGET_HEIGHT * 3);
  if (!resized) {
    stbi_image_free(img);
    return 1;
  }

  stbir_resize_uint8_linear(img, original_w, original_h, 0, resized, TARGET_WIDTH, TARGET_HEIGHT, 0, STBIR_RGB);

  stbi_image_free(img);

  // Binary array
  uint8_t binary_array[TARGET_HEIGHT][TARGET_WIDTH];

  for (int y = 0; y < TARGET_HEIGHT; y++) {
    for (int x = 0; x < TARGET_WIDTH; x++) {
      int idx = (y * TARGET_WIDTH + x) * 3;

      uint8_t r = resized[idx + 0];
      uint8_t g = resized[idx + 1];
      uint8_t b = resized[idx + 2];

      float brightness = 0.299f * r + 0.587f * g + 0.114f * b;

      if (brightness > 128.0f)
        binary_array[y][x] = 1;
      else
        binary_array[y][x] = 0;
    }
  }

  free(resized);

  // First pass: count max runs
  int run_counts[TARGET_HEIGHT];
  int max_runs = 0;

  for (int y = 0; y < TARGET_HEIGHT; y++) {
    int x = 0;
    int count = 0;

    while (x < TARGET_WIDTH) {
      // skip white pixels
      while (x < TARGET_WIDTH && binary_array[y][x] == 1) x++;

      if (x >= TARGET_WIDTH) break;

      // start of black run
      int start = x;

      while (x < TARGET_WIDTH && binary_array[y][x] == 0) x++;

      int length = x - start;
      (void)length;  // only counting here
      count++;
    }

    run_counts[y] = count;
    if (count > max_runs) max_runs = count;
  }

  // Allocate runs
  Run image_runs[TARGET_HEIGHT][max_runs];

  // Initialize to {0,0}
  for (int y = 0; y < TARGET_HEIGHT; y++) {
    for (int i = 0; i < max_runs; i++) {
      image_runs[y][i].start = 0;
      image_runs[y][i].length = 0;
    }
  }

  // Second pass: fill runs
  for (int y = 0; y < TARGET_HEIGHT; y++) {
    int x = 0;
    int run_index = 0;

    while (x < TARGET_WIDTH) {
      while (x < TARGET_WIDTH && binary_array[y][x] == 1) x++;

      if (x >= TARGET_WIDTH) break;

      int start = x;

      while (x < TARGET_WIDTH && binary_array[y][x] == 0) x++;

      int length = x - start;

      image_runs[y][run_index].start = (uint8_t)start;
      image_runs[y][run_index].length = (uint8_t)length;
      run_index++;
    }
  }

  // Write header file
  FILE* f = fopen("../Inc/image_data.h", "w");
  if (!f) {
    printf("Failed to open output file\n");
    return 1;
  }

  fprintf(f, "#ifndef IMAGE_DATA_H\n");
  fprintf(f, "#define IMAGE_DATA_H\n\n");
  fprintf(f, "#include <stdint.h>\n\n");

  fprintf(f, "#define IMAGE_WIDTH %d\n", TARGET_WIDTH);
  fprintf(f, "#define IMAGE_HEIGHT %d\n", TARGET_HEIGHT);
  fprintf(f, "#define MAX_RUNS %d\n\n", max_runs);

  fprintf(f, "typedef struct {\n");
  fprintf(f, "    uint8_t start;\n");
  fprintf(f, "    uint8_t length;\n");
  fprintf(f, "} Run;\n\n");

  fprintf(f, "const Run image_runs[IMAGE_HEIGHT][MAX_RUNS] = {\n");
  for (int y = 0; y < TARGET_HEIGHT; y++) {
    fprintf(f, "    {");
    for (int i = 0; i < max_runs; i++) {
      fprintf(f, "{%d,%d}", image_runs[y][i].start, image_runs[y][i].length);
      if (i < max_runs - 1) fprintf(f, ",");
    }
    fprintf(f, "},\n");
  }
  fprintf(f, "};\n\n");

  fprintf(f, "const uint8_t run_counts[IMAGE_HEIGHT] = {");
  for (int y = 0; y < TARGET_HEIGHT; y++) {
    fprintf(f, "%d", run_counts[y]);
    if (y < TARGET_HEIGHT - 1) fprintf(f, ",");
  }
  fprintf(f, "};\n\n");

  fprintf(f, "#endif\n");

  fclose(f);

  printf("image_data.h generated successfully.\n");
  return 0;
}