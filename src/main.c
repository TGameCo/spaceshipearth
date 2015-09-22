#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_ampm_layer;
static TextLayer *s_date_layer;
static BitmapLayer *s_pattern_layer;
static BitmapLayer *s_ampm_backer_layer_white, *s_ampm_backer_layer_black;
static BitmapLayer *s_time_backer_layer_white, *s_time_backer_layer_black;
static BitmapLayer *s_date_backer_layer_white, *s_date_backer_layer_black;
static BitmapLayer *s_over_backer_layer_white, *s_over_backer_layer_black;
static InverterLayer *s_invert_layer;
static Layer *s_batteryIndicator_layer;
static GBitmap *s_pattern_bitmap;
static GBitmap *s_ampm_bitmap_white, *s_ampm_bitmap_black;
static GBitmap *s_time_bitmap_white, *s_time_bitmap_black;
static GBitmap *s_date_bitmap_white, *s_date_bitmap_black;
static GBitmap *s_over_bitmap_white, *s_over_bitmap_black;
static PropertyAnimation *s_spinAnimation;
static int s_battery_level;
static int animCount;
static GRect patternStart;
static GRect patternEnd;
static bool spinning;

static void spinAgain();

void stopped_spin_animation(Animation *animation, void *data){
  //property_animation_destroy(s_spinAnimation);
  layer_set_frame(bitmap_layer_get_layer(s_pattern_layer), patternStart);
  if(animCount > 0){
    animCount -= 1;
    spinning = false;
    spinAgain();
    
    APP_LOG(APP_LOG_LEVEL_INFO, "SPINNING AGAIN");
  } else {
    spinning = false;
  }
  
  //APP_LOG(APP_LOG_LEVEL_INFO, "Ended spinning");
}

void started_spin_animation(Animation *animation, void *data){
  APP_LOG(APP_LOG_LEVEL_INFO, "Begun spinning");
  spinning = true;
}

static void trigger_spin_animation(){
  if(spinning == true){
    APP_LOG(APP_LOG_LEVEL_INFO, "Tried to spin, can't");
    return;
  }
  
  //Set start and end
  GRect from_frame = patternStart;
  GRect to_frame = patternEnd;
  
  
  s_spinAnimation = property_animation_create_layer_frame(bitmap_layer_get_layer(s_pattern_layer), &from_frame, &to_frame);
  animation_set_duration((Animation*)s_spinAnimation, 1000);
  if(animCount >= 1){
    animation_set_curve((Animation*)s_spinAnimation, AnimationCurveLinear);
  } else {
    animation_set_curve((Animation*)s_spinAnimation, AnimationCurveEaseOut);
  }
  
  animation_set_handlers((Animation*) s_spinAnimation, (AnimationHandlers){
    .started = (AnimationStartedHandler) started_spin_animation,
    .stopped = (AnimationStoppedHandler) stopped_spin_animation,
  }, NULL);
  
  if (spinning == false){
  APP_LOG(APP_LOG_LEVEL_INFO, "AnimationTriggered");
  animation_schedule((Animation*) s_spinAnimation);
  }
}

static void spinAgain(){
  trigger_spin_animation();
};

static void tap_handler(AccelAxisType axis, int32_t direction) {
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      //animCount = 1;
      //trigger_spin_animation();
    } else {
      //animCount = 1;
      //trigger_spin_animation();
    }
    break;
  case ACCEL_AXIS_Y:
    if (direction > 0) {
      animCount = 1;
      trigger_spin_animation();
      //APP_LOG(APP_LOG_LEVEL_INFO, "Triggered the spin animation");
    } else {
      animCount = 1;
      trigger_spin_animation();
      //APP_LOG(APP_LOG_LEVEL_INFO, "Triggered the spin animation2");
    }
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      //animCount = 1;
      //trigger_spin_animation();
      //APP_LOG(APP_LOG_LEVEL_ERROR, "TopLeft");
    } else {
      //animCount = 1;
      //trigger_spin_animation();
    }
    break;
  }
}

static void update_time() {
  //Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  //Create a long-lived buffer
  static char buffer[] = "00:00";
  static char ampm[] = "PM";
  static char date[] = "WED 00";
  static char hour[] = "00";
  static int intH;
  
  //Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 24 hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    strftime(hour, sizeof("00"), "%H", tick_time);
    intH = atoi(hour);
    if(8 < intH && intH < 20){
      layer_set_hidden(inverter_layer_get_layer(s_invert_layer), true);
    } else {
      layer_set_hidden(inverter_layer_get_layer(s_invert_layer), false);
    }
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    strftime(ampm, sizeof("PM"), "%p", tick_time);
    strftime(hour, sizeof("00"), "%I", tick_time);
    intH = atoi(hour);
    APP_LOG(APP_LOG_LEVEL_INFO, hour);
    if(strcmp(ampm, "PM") == 0){
      if(intH >= 8 && intH < 12){
        APP_LOG(APP_LOG_LEVEL_INFO, "Night");
        layer_set_hidden(inverter_layer_get_layer(s_invert_layer), false);
      } else {
        layer_set_hidden(inverter_layer_get_layer(s_invert_layer), true);
      }
    } else {
      if(intH <= 8){
        layer_set_hidden(inverter_layer_get_layer(s_invert_layer), false);
      } else {
        layer_set_hidden(inverter_layer_get_layer(s_invert_layer), true);
      }
    }
    text_layer_set_text(s_ampm_layer, ampm);
    
  }
  strftime(date, sizeof("WED 00"), "%b %e", tick_time);
  
  //Display on the text
  text_layer_set_text(s_date_layer, date);
  text_layer_set_text(s_time_layer, buffer);
  //trigger_spin_animation();
}

static void battery_callback(BatteryChargeState state){
  //Get new battery level;
  s_battery_level = state.charge_percent;
  layer_mark_dirty(s_batteryIndicator_layer);
}

static void update_battery(Layer *thisLayer, GContext *ctx) {
  GRect bounds = layer_get_bounds(thisLayer);
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 120.0F);
  
  //Draw the bar
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void make_transparent_backer(GRect size, GBitmap *white_image, GBitmap *black_image, Layer *parentWindow, BitmapLayer *bitmapLayerWhite, BitmapLayer *bitmapLayerBlack){
  bitmapLayerWhite = bitmap_layer_create(size);
  bitmapLayerBlack = bitmap_layer_create(size);
  bitmap_layer_set_bitmap(bitmapLayerWhite, white_image);
  bitmap_layer_set_bitmap(bitmapLayerBlack, black_image);
  bitmap_layer_set_compositing_mode(bitmapLayerWhite, GCompOpOr);
  bitmap_layer_set_compositing_mode(bitmapLayerBlack, GCompOpClear);
  layer_add_child(parentWindow, bitmap_layer_get_layer(bitmapLayerWhite));
  layer_add_child(parentWindow, bitmap_layer_get_layer(bitmapLayerBlack));
}

static void main_window_load(Window *window){
  Layer *windowLayer = window_get_root_layer(s_main_window);
  patternStart = GRect(-29, -29, 169, 193);
  patternEnd = GRect(-11, -11, 169, 193);
  GRect fullFrame = GRect(0, 0, 144, 168);
  //Create pattern
  s_pattern_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PATTERN_BACKER);
  s_pattern_layer = bitmap_layer_create(patternStart);
  bitmap_layer_set_bitmap(s_pattern_layer, s_pattern_bitmap);
  layer_add_child(windowLayer, bitmap_layer_get_layer(s_pattern_layer));
  
  GRect ampmFrame = GRect(9, 117, 50, 27);
  GRect timeFrame = GRect(4, 74, 136, 44);
  GRect dateFrame = GRect(61, 117, 73, 27);
  s_ampm_bitmap_white = gbitmap_create_with_resource(RESOURCE_ID_AMPM_BACKER_WHITE);
  s_ampm_bitmap_black = gbitmap_create_with_resource(RESOURCE_ID_AMPM_BACKER_BLACK);
  s_time_bitmap_white = gbitmap_create_with_resource(RESOURCE_ID_TIME_BACKER_WHITE);
  s_time_bitmap_black = gbitmap_create_with_resource(RESOURCE_ID_TIME_BACKER_BLACK);
  s_date_bitmap_white = gbitmap_create_with_resource(RESOURCE_ID_DATE_BACKER_WHITE);
  s_date_bitmap_black = gbitmap_create_with_resource(RESOURCE_ID_DATE_BACKER_BLACK);
  s_over_bitmap_white = gbitmap_create_with_resource(RESOURCE_ID_OVER_BACKER_WHITE);
  s_over_bitmap_black = gbitmap_create_with_resource(RESOURCE_ID_OVER_BACKER_BLACK);
  
  //Making the overbacker
  make_transparent_backer(fullFrame,
                          s_over_bitmap_white, 
                          s_over_bitmap_black, 
                          windowLayer, 
                          s_over_backer_layer_white, 
                          s_over_backer_layer_black);
  
  
  //Making the AMPM backer
  if(clock_is_24h_style() == false){
    make_transparent_backer(ampmFrame, 
                          s_ampm_bitmap_white,
                          s_ampm_bitmap_black,
                          windowLayer,
                          s_ampm_backer_layer_white, 
                          s_ampm_backer_layer_black);
  }
  
  //Making the date backer
  make_transparent_backer(dateFrame,
                         s_date_bitmap_white,
                         s_date_bitmap_black,
                         windowLayer,
                         s_date_backer_layer_white,
                         s_date_backer_layer_black);
  
  //Making the time backer
  make_transparent_backer(timeFrame, 
                          s_time_bitmap_white, 
                          s_time_bitmap_black, 
                          windowLayer, 
                          s_time_backer_layer_white, 
                          s_time_backer_layer_black);
  

  
  //Setting up battery indicator bar
  s_batteryIndicator_layer = layer_create(GRect(12, 75, 120, 3));
  layer_set_update_proc(s_batteryIndicator_layer, update_battery);
  layer_add_child(windowLayer, s_batteryIndicator_layer);
  
  //Setting up text layers
  s_time_layer = text_layer_create(GRect(5, 66, 139, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  
  s_ampm_layer = text_layer_create(GRect(9, 117, 50, 27));
  text_layer_set_background_color(s_ampm_layer, GColorClear);
  text_layer_set_text_color(s_ampm_layer, GColorBlack);
  text_layer_set_text(s_ampm_layer, "PM");
  text_layer_set_font(s_ampm_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_ampm_layer, GTextAlignmentCenter);
  
  s_date_layer = text_layer_create(dateFrame);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_text(s_date_layer, "00/00/00");
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  
  //Improve text layer
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
  
  //Setting up the invert layer
  s_invert_layer = inverter_layer_create(fullFrame);
  //layer_set_hidden(inverter_layer_get_layer(s_invert_layer));
  
  //Add it as a child layer to the window's root layer
  if(clock_is_24h_style() == false){
    layer_add_child(windowLayer, text_layer_get_layer(s_ampm_layer));
  }
  layer_add_child(windowLayer, text_layer_get_layer(s_time_layer));
  layer_add_child(windowLayer, text_layer_get_layer(s_date_layer));
  
  //ALWAYS ON TOP
  layer_add_child(windowLayer, inverter_layer_get_layer(s_invert_layer));
  
  //Make sure time is displayed
  update_time();
  //trigger_spin_animation();
}

static void main_window_unload(Window *window){
  //Destroy GBitmap
  gbitmap_destroy(s_pattern_bitmap);
  gbitmap_destroy(s_ampm_bitmap_black);
  gbitmap_destroy(s_ampm_bitmap_white);
  gbitmap_destroy(s_time_bitmap_black);
  gbitmap_destroy(s_time_bitmap_white);
  gbitmap_destroy(s_date_bitmap_black);
  gbitmap_destroy(s_date_bitmap_white);
  gbitmap_destroy(s_over_bitmap_black);
  gbitmap_destroy(s_over_bitmap_white);
  
  //Destroy Bitmaplayer
  bitmap_layer_destroy(s_pattern_layer);
  bitmap_layer_destroy(s_ampm_backer_layer_white);
  bitmap_layer_destroy(s_ampm_backer_layer_black);
  bitmap_layer_destroy(s_time_backer_layer_black);
  bitmap_layer_destroy(s_time_backer_layer_white);
  bitmap_layer_destroy(s_date_backer_layer_black);
  bitmap_layer_destroy(s_date_backer_layer_white);
  bitmap_layer_destroy(s_over_backer_layer_white);
  bitmap_layer_destroy(s_over_backer_layer_black);
  
  //Destroy textlayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_ampm_layer);
  text_layer_destroy(s_date_layer);
  
  //Destroy Things
  layer_destroy(s_batteryIndicator_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}

static void init() {
  //Create the main window element and assign it to the pointer
  s_main_window = window_create();
  
  //Set handlers to manage the events inside the folder
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  animCount = 1;
  //Show the window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  //Registering the Timer
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  //Registering the tapper
  accel_tap_service_subscribe(tap_handler);
  
  //Registering battery
  battery_callback(battery_state_service_peek());
}

static void deinit() {
  //Kill window
  window_destroy(s_main_window);
}

int main(void){
  init();
  app_event_loop();
  deinit();
}