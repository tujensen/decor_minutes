/**
  Decor Minutes
  written by Troels Ugilt Jensen.
  http://tuj.dk
*/

#include <pebble.h>
  
#define MAX_COLORS 9
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_info_layer;
static GFont s_time_font;
static Layer *s_canvas;

static GPath *battery;

static const int colors[MAX_COLORS] = {
  0x555555, // GColorDarkGray
  0x00AAAA, // GColorTiffanyBlue
  0x5555AA, // GColorLiberty
  0x00AA55, // GColorJaegerGreen
  0xAA00FF, // GColorVividViolet
  0xFF5500, // GColorOrange
  0x555500, // GColorArmyGreen
  0xFF5555, // GColorSunsetOrange
  0x0055AA, // GColorCobaltBlue
};

static const int background_colors[MAX_COLORS] = {
  0xAAAAAA, // GColorLightGray
  0x55FFFF, // GColorElectricBlue
  0xAAAAFF, // GColorBabyBlueEyes
  0xAAFF55, // GColorInchworm
  0xFF55FF, // GColorShockingPink
  0xFFAA00, // GColorChromeYellow
  0xAAAA00, // GColorLimerick
  0xFFAAAA, // GColorMelon
  0x00AAFF, // GColorVividCerulean
};

static const GPathInfo BATTERY_PATH_INFO = {
  .num_points = 9,
  .points = (GPoint []) {{125, 3}, {136, 3}, {136, 4}, {137, 4}, {137, 8}, {136, 8}, {136, 9}, {125, 9}, {125, 3}}
};

int selected_color;
int battery_level;
int box_width;

static void draw_battery(GContext *ctx, GColor8 color) {
  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_fill_color(ctx, color);
  gpath_draw_filled(ctx, battery);
  
  graphics_context_set_stroke_color(ctx, GColorFromHEX(0x000000));
  gpath_draw_outline(ctx, battery);

  graphics_context_set_stroke_color(ctx, GColorFromHEX(0xFFFFFF));
  graphics_context_set_stroke_width(ctx, 1);
  gpath_draw_outline(ctx, battery);
}

static void update_canvas(Layer *this_layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorFromHEX(colors[selected_color]));
  graphics_fill_rect(ctx, GRect(0, 0, box_width, 168), 0, GCornerNone);
  
  // Show battery low indicator below 25 %
  if (battery_level < 25) {
    draw_battery(ctx, GColorFromHEX(0xFF0000));
  }
}

static void update_time(int force) {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char sbuffer[] = "00";
  strftime(sbuffer, sizeof("00"), "%S", tick_time);
    
  // Get seconds as int
  int result = atoi(sbuffer);
  
  box_width = 144 * result / 60 + 2;
  layer_mark_dirty(s_canvas);
  
  // Did a minute pass, or was force == true?
  if (result <= 0 || force) {
    static char buffer[] = "00:00";
    
    // Write the current hours and minutes into the buffer
    if (clock_is_24h_style() == true) {
      // Use 24 hour format
      strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
      // Use 12 hour format
      strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }

    // Update date text.
    static char dbuffer[] = "";
    strftime(dbuffer, 80, "%a. %d. %b. %Y", tick_time);
    text_layer_set_text(s_info_layer, dbuffer);

    // Set the minute texts.
    text_layer_set_text(s_time_layer, buffer);

    // Get new color theme.
    selected_color = rand() % MAX_COLORS;
    window_set_background_color(s_main_window, GColorFromHEX(background_colors[selected_color]));
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time(0);
}

static void battery_handler(BatteryChargeState new_state) {
  // Write to buffer and display
  static char s_battery_buffer[32];
  snprintf(s_battery_buffer, sizeof(s_battery_buffer), "%d", new_state.charge_percent);
  battery_level = atoi(s_battery_buffer);
}

static void main_window_load(Window *window) {
  // Initialize variables.
  selected_color = 0;
  
  // Set background color.
  window_set_background_color(s_main_window, GColorFromHEX(background_colors[selected_color]));

  // Create background canvas.
  s_canvas = layer_create(GRect(0, 0, 144, 168));
  
  // Create watch texts.
  s_time_layer = text_layer_create(GRect(0, 30, 144, 54));
  s_info_layer = text_layer_create(GRect(10, 134, 124, 24));
  
  // Setup watch text colors.
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_info_layer, GColorBlack);
  text_layer_set_background_color(s_info_layer, GColorClear);
  
  // Setup font for watch.
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTICON_42));
  text_layer_set_font(s_time_layer, s_time_font);

  text_layer_set_font(s_info_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  
  // Set text alignments.
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text_alignment(s_info_layer, GTextAlignmentCenter);
  
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), s_canvas);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_info_layer));

  // Get the current battery level
  battery_handler(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_info_layer);

  gpath_destroy(battery);
  
  layer_destroy(s_canvas);

  // Unload GFont
  fonts_unload_custom_font(s_time_font);
}

static void init() {
  srand(time(NULL));
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  update_time(1);
  
  // Battery GPath.
  battery = gpath_create(&BATTERY_PATH_INFO);
  
  // Set update procedure for s_canvas.
  layer_set_update_proc(s_canvas, update_canvas);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  
  // Subscribe to the Battery State Service
  battery_state_service_subscribe(battery_handler);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}