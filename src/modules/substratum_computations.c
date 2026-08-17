#include "substratum_computations.h"

#include "modules/helper_computations.h"

// Scale icon's X relative to the current frame vs. the reference design
int16_t substratum_renderer_scale_icon_x(
    const GSize* size,
    int16_t coord) {
  if (!size || !SUBSTRATUM_VALID_DESIGN_X(coord)) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, size->w, WATCHFACE_ICON_GRID_WIDTH),
      (size->w - 1));
}

// Scale icon's Y relative to the current frame vs. the reference design
int16_t substratum_renderer_scale_icon_y(
    const GSize* size,
    int16_t coord) {
  if (!size || !SUBSTRATUM_VALID_DESIGN_Y(coord)) {
    return 0;
  }

  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, size->h, WATCHFACE_ICON_GRID_HEIGHT),
      (size->h - 1));
}

int16_t substratum_renderer_scale_icon_coord(
    const GSize* size,
    int16_t coord) {
  if (!size) {
    return 0;
  }

  if (!(SUBSTRATUM_VALID_DESIGN_X(coord) || SUBSTRATUM_VALID_DESIGN_Y(coord))) {
    return 0;
  }

  int16_t chosen_dimension = HELPER_MIN(size->w, size->h);
  int16_t design_dimension =
      (chosen_dimension == size->w) ? WATCHFACE_ICON_GRID_WIDTH : WATCHFACE_ICON_GRID_HEIGHT;
  // Clamp so coords go from 0..total-1
  return HELPER_CLAMP_MAX(
      HELPER_SCALE_ROUND(coord, chosen_dimension, design_dimension),
      (chosen_dimension - 1));
}

GPoint substratum_renderer_scale_icon_point(
    const GSize* size,
    int16_t x,
    int16_t y) {
  if (!(SUBSTRATUM_VALID_DESIGN_X(x) && SUBSTRATUM_VALID_DESIGN_Y(y))) {
    // Return the origin of the frame; at least something will get drawn
    return GPoint(0, 0);
  }

  if (size) {
    return GPoint(
        substratum_renderer_scale_icon_x(size, x),
        substratum_renderer_scale_icon_y(size, y));
  }

  return GPoint(x, y);
}

int16_t substratum_renderer_scale_icon_x_in_frame(
    const GRect* frame,
    int16_t coord) {
  if (!frame) {
    return 0;
  }

  return HELPER_CLAMP_TO_RANGE(
      frame->origin.x + substratum_renderer_scale_icon_x(&frame->size, coord),
      frame->origin.x,
      HELPER_MIN((frame->origin.x + frame->size.w - 1), (PBL_DISPLAY_WIDTH - 1)));
}

int16_t substratum_renderer_scale_icon_y_in_frame(
    const GRect* frame,
    int16_t coord) {
  if (!frame) {
    return 0;
  }

  return HELPER_CLAMP_TO_RANGE(
      frame->origin.y + substratum_renderer_scale_icon_y(&frame->size, coord),
      frame->origin.y,
      HELPER_MIN((frame->origin.y + frame->size.h - 1), (PBL_DISPLAY_HEIGHT - 1)));
}

void substratum_renderer_scale_icon_point_in_frame(
    const GRect* frame,
    GPoint* input) {
  input->x = substratum_renderer_scale_icon_x_in_frame(frame, input->x);
  input->y = substratum_renderer_scale_icon_y_in_frame(frame, input->y);
}

GPoint substratum_renderer_scale_icon_x_y_in_frame(
    const GRect* frame,
    int16_t x,
    int16_t y) {
  return GPoint(
      substratum_renderer_scale_icon_x_in_frame(frame, x),
      substratum_renderer_scale_icon_y_in_frame(frame, y));
}

void substratum_renderer_create_subframe(
    const GRect* frame,
    GRect* out,
    int16_t x,
    int16_t y,
    int16_t w,
    int16_t h) {
  if (!frame || !out) {
    return;
  }

  // Set the point to be as it would have been in the reference design
  // Once you've set this up, you can scale the point's coordinates
  // to the frame's width and height
  out->origin.x = x;
  out->origin.y = y;
  substratum_renderer_scale_icon_point_in_frame(frame, &out->origin);
  int16_t maybe_w = HELPER_SCALE_ROUND(w, frame->size.w, WATCHFACE_ICON_GRID_WIDTH);
  int16_t maybe_h = HELPER_SCALE_ROUND(h, frame->size.h, WATCHFACE_ICON_GRID_HEIGHT);

  // If origin.x + w is greater than the screen's width, clamp width to be
  // PBL_DISPLAY_WIDTH-origin.x. Same for height calculation.
  out->size.w = HELPER_MIN(maybe_w, PBL_DISPLAY_WIDTH - out->origin.x);
  out->size.h = HELPER_MIN(maybe_h, PBL_DISPLAY_HEIGHT - out->origin.y);
}
