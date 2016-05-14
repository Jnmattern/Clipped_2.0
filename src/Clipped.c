#include <pebble.h>

// Languages
#define LANG_DUTCH 0
#define LANG_ENGLISH 1
#define LANG_FRENCH 2
#define LANG_GERMAN 3
#define LANG_SPANISH 4
#define LANG_PORTUGUESE 5
#define LANG_SWEDISH 6
#define LANG_ITALIAN 7
#define LANG_DANISH 8
#define LANG_MAX 9

enum {
  CONFIG_KEY_DATEORDER		= 90,
  CONFIG_KEY_WEEKDAY		= 91,
  CONFIG_KEY_LANG       = 92,
  CONFIG_KEY_BIGMINUTES 	= 93,
  CONFIG_KEY_SHOWDATE		= 94,
  CONFIG_KEY_NEGATIVE   = 95,
  CONFIG_KEY_THEMECODE  = 96,
  CONFIG_KEY_BTALERT    = 97
};

// Screen dimensions
#if defined(PBL_RECT)
  #define SCREENW 144
  #define SCREENH 168
  #define CX      72
  #define CY      84
  #define TEXTX   1
  #define TEXTW   142
#elif defined(PBL_ROUND)
  #define SCREENW 180
  #define SCREENH 180
  #define CX      90
  #define CY      90
  #define TEXTX   11
  #define TEXTW   160
#endif
// Date Layer Frame
#define TEXTY 96
#define TEXTH 60

// Space between big digits
#define BIGDIGITS_PADDING 6


#define VOFFSET_DATE 0
#define VOFFSET_NODATE 15

#define BATTERY_STATUS_WIDTH 76
#define BATTERY_STATUS_HEIGHT 36


int vOffset = VOFFSET_DATE;

// IDs of the images for the big digits
const int digitImage[10] = {
  RESOURCE_ID_IMAGE_D0, RESOURCE_ID_IMAGE_D1, RESOURCE_ID_IMAGE_D2, RESOURCE_ID_IMAGE_D3,
  RESOURCE_ID_IMAGE_D4, RESOURCE_ID_IMAGE_D5, RESOURCE_ID_IMAGE_D6, RESOURCE_ID_IMAGE_D7,
  RESOURCE_ID_IMAGE_D8, RESOURCE_ID_IMAGE_D9
};

// Days of the week in all languages
const char weekDay[LANG_MAX][7][6] = {
  { "zon", "maa", "din", "woe", "don", "vri", "zat" },	// Dutch
  { "sun", "mon", "tue", "wed", "thu", "fri", "sat" },	// English
  { "dim", "lun", "mar", "mer", "jeu", "ven", "sam" },	// French
  { "son", "mon", "die", "mit", "don", "fre", "sam" },	// German
  { "dom", "lun", "mar", "mie", "jue", "vie", "sab" },	// Spanish
  { "dom", "seg", "ter", "qua", "qui", "sex", "sab" },	// Portuguese
  { "sön", "mån", "tis", "ons", "tor", "fre", "lör" },// Swedish
  { "dom", "lun", "mar", "mer", "gio", "ven", "sab" },	// Italian
  { "søn", "man", "tir", "ons", "tor", "fre", "lør" }	// Danish
};


char buffer[256] = "";
int curLang = LANG_ENGLISH;
int showWeekday = 1;
int USDate = 1;
int showDate = 1;
int bigMinutes = 0;
int bluetoothStatus = 1;
int negative = 0;
int btalert = 1;
static char themeCodeText[20] = "c0fef8c0fd";

bool configChanged = false;
bool lastBluetoothStatus = true;


// Structure to hold informations for the two big digits
typedef struct {
  GBitmap *bitmap;	// the bitmap to display
  GRect frame;		// the frame in which the layer is positionned
  int curDigit;		// Current digit to display
  int prevDigit;		// Previous digit displayed
} bigDigit;

// Main window
Window *window;
// Main window layer
Layer *rootLayer;
// Background layer which will receive the update events
Layer *bgLayer;
// the two big digits structures
bigDigit bigSlot[2];
// TextLayers for the small digits and the date
// There are 5 layers for the small digits to simulate outliningof the font (4 layers to the back & 1 to the front)
#define SMALLDIGITSLAYERS_NUM 5
TextLayer *smallDigitLayer[SMALLDIGITSLAYERS_NUM], *dateLayer = NULL;
// Info layer
TextLayer *infoLayer = NULL;
enum {
  BATTERY = 1,
  BLUETOOTH_ON = 2,
  BLUETOOTH_OFF = 3
};

// The custom font
GFont customFont;
// String for the small digits
char smallDigits[] = "00";
// String for the date
char date[12] = "012345678901";
// various compute variables
char D[2], M[2];
int wd;
// Is clock in 12h format ?
bool clock12 = false;
// x&y offsets for the SMALLDIGITSLAYERS_NUM TextLayers for the small digits to simulate outlining
int dx[SMALLDIGITSLAYERS_NUM] = { -2, 2, 2, -2, 0 };
int dy[SMALLDIGITSLAYERS_NUM] = { -2, -2, 2, 2, 0 };

GColor textColor[SMALLDIGITSLAYERS_NUM];
GColor colors[5];

// Current and previous timestamps, last defined to -1 to be sure to update at launch
time_t now;
struct tm *curTime;
struct tm last = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "tz" };


static void bgLayerUpdateProc(struct Layer *layer, GContext* ctx) {
  int i;

  for (i=0; i<2; i++) {
    if (bigSlot[i].bitmap != NULL) {
      graphics_draw_bitmap_in_rect(ctx, bigSlot[i].bitmap, bigSlot[i].frame);
    }
  }
}

// big digits update procedure
static void updateBigDigits(int val) {
  int i, width = BIGDIGITS_PADDING; // padding between the two big digits
  int d[2];

  d[0] = val/10;  // tens
  d[1] = val%10;  // units

  // foreach big digit slot
  for (i=0; i<2; i++) {
    // Backup previous digits, used to check if they changed
    bigSlot[i].prevDigit = bigSlot[i].curDigit;
    bigSlot[i].curDigit = d[i];

    if (bigSlot[i].curDigit != bigSlot[i].prevDigit) {
      // if the digit has changed, remove old bitmap and load new one
      if (bigSlot[i].bitmap != NULL) {
        gbitmap_destroy(bigSlot[i].bitmap);
      }
      bigSlot[i].bitmap = gbitmap_create_with_resource(digitImage[bigSlot[i].curDigit]);
      bigSlot[i].frame = gbitmap_get_bounds(bigSlot[i].bitmap);

      gbitmap_set_palette(bigSlot[i].bitmap, colors, false);
    }
    // Calculate the total width of the two digits so to center them afterwards:
    // they can be different widths so they're not aligned to the center of the screen
    width += bigSlot[i].frame.size.w;
  }

  // Offset the first digit to the left of half the calculated width starting from the middle of the screen
  bigSlot[0].frame.origin.x = CX - (width/2);
  // Offset the second digit to the right of the first one
  bigSlot[1].frame.origin.x = bigSlot[0].frame.origin.x + bigSlot[0].frame.size.w + BIGDIGITS_PADDING;

  layer_mark_dirty(bgLayer);
}

static void updateSmallDigits(int val) {
  int i;

  smallDigits[0] = '0' + (char)(val/10);
  smallDigits[1] = '0' + (char)(val%10);

  for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
    // Set small digits TextLayers's text, this triggers a redraw
    text_layer_set_text(smallDigitLayer[i], smallDigits);
  }
}

// global time variables handler
static void setHM(struct tm *tm) {
  int h;

  if ((tm->tm_hour != last.tm_hour) || configChanged) {
    h = tm->tm_hour;
    if (clock12) {
      h = h%12;
      if (h == 0) {
        h = 12;
      }
    }

    // Hour digits
    if (bigMinutes) {
      // Set small digits string to hours
      updateSmallDigits(h);
    } else {
      // Set big digits to hours
      updateBigDigits(h);
    }
  }

  if ((tm->tm_min != last.tm_min) || configChanged) {
    if (bigMinutes) {
      // Set big digits to minutes
      updateBigDigits(tm->tm_min);
    } else {
      // Set small digits string to minutes
      updateSmallDigits(tm->tm_min);
    }
  }

  if (showDate) {
    if ((tm->tm_mday != last.tm_mday) || configChanged) {
      // Date Layer string formatting

      // Get day of month
      D[0] = (char)(tm->tm_mday/10);
      D[1] = (char)(tm->tm_mday%10);

      // Get month num
      if ((tm->tm_mon != last.tm_mon) || configChanged) {
        M[0] = (char)((tm->tm_mon+1)/10);
        M[1] = (char)((tm->tm_mon+1)%10);
      }

      // Get day of week
      if ((tm->tm_wday != last.tm_wday) || configChanged) {
        wd = tm->tm_wday;
      }

      if (showWeekday) {
        // Day of week formatting : "www dd"
        snprintf(date, 12, "%s %.2d", weekDay[curLang][wd], (int)tm->tm_mday);
      } else {
        if (USDate) {
          // US date formatting : "mm dd"
          snprintf(date, 12, "%.2d %.2d", (int)(tm->tm_mon+1), (int)tm->tm_mday);
        } else {
          // EU date formatting : "dd mm"
          snprintf(date, 12, "%.2d %.2d", (int)tm->tm_mday, (int)(tm->tm_mon+1));
        } // if (USDate)
      } // if (showWeekday)
        // Set date TextLayers's text, this triggers a redraw
      text_layer_set_text(dateLayer, date);
    }
  } // if (showDate)

  // Backup current time
  last = *tm;
  configChanged = false;
}

// time event handler, triggered every minute
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  setHM(tick_time);
}

static void createDateLayer(void) {
  GRect b;
  if (dateLayer == NULL) {
    vOffset = VOFFSET_DATE;
    b = layer_get_bounds(bgLayer);
    b.origin.y = vOffset;
    layer_set_bounds(bgLayer, b);

    dateLayer = text_layer_create(GRect(-20, 134, SCREENW+40, TEXTH));
    text_layer_set_background_color(dateLayer, GColorClear);
    text_layer_set_font(dateLayer, customFont);
    text_layer_set_text_alignment(dateLayer, GTextAlignmentCenter);
    text_layer_set_text_color(dateLayer, colors[4]);
    text_layer_set_text(dateLayer, date);
    layer_add_child(rootLayer, text_layer_get_layer(dateLayer));
  }
}

static void destroyDateLayer(void) {
  GRect b;
  if (dateLayer != NULL) {
    vOffset = VOFFSET_NODATE;
    b = layer_get_bounds(bgLayer);
    b.origin.y = vOffset;
    layer_set_bounds(bgLayer, b);

    text_layer_destroy(dateLayer);
    dateLayer = NULL;
  }
}

int hexCharToInt(const char digit) {
  if ((digit >= '0') && (digit <= '9')) {
    return (int)(digit - '0');
  } else if ((digit >= 'a') && (digit <= 'f')) {
    return 10 + (int)(digit - 'a');
  } else if ((digit >= 'A') && (digit <= 'F')) {
    return 10 + (int)(digit - 'A');
  } else {
    return -1;
  }
}

int hexStringToByte(const char *hexString) {
  int l = strlen(hexString);
  if (l == 0) return 0;
  if (l == 1) return hexCharToInt(hexString[0]);

  return 16*hexCharToInt(hexString[0]) + hexCharToInt(hexString[1]);
}

void decodeThemeCode(char *code) {
#if defined(PBL_COLOR)
  int i;

  for (i=0; i<5; i++) {
    colors[i] = (GColor8){.argb=(uint8_t)hexStringToByte(code + 2*i)};
  }
#endif
}

static void setColors() {
#if defined(PBL_BW)
  if (negative) {
    colors[0] = GColorWhite;        // Background
    colors[1] = GColorBlack; // Big digits
    colors[2] = GColorWhite;          // Small digits
    colors[3] = GColorBlack;        // Small digits outline
    colors[4] = GColorBlack;         // Date
  } else {
    colors[0] = GColorBlack;        // Background
    colors[1] = GColorWhite; // Big digits
    colors[2] = GColorBlack;          // Small digits
    colors[3] = GColorWhite;        // Small digits outline
    colors[4] = GColorWhite;         // Date
  }
#elif defined(PBL_COLOR)
  decodeThemeCode(themeCodeText);
#endif

  textColor[0] = textColor[1] = textColor[2] = textColor[3] = colors[3];
  textColor[4] = colors[2];
}

static void applyConfig() {
  int i;

  if (showDate) {
    createDateLayer();
  } else {
    destroyDateLayer();
  }

  for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
    if (bigMinutes) {
      text_layer_set_text_alignment(smallDigitLayer[i], GTextAlignmentLeft);
    } else {
      text_layer_set_text_alignment(smallDigitLayer[i], GTextAlignmentRight);
    }
  }

  setColors();

  window_set_background_color(window, colors[0]);

  for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
    text_layer_set_text_color(smallDigitLayer[i], textColor[i]);
  }

  if (dateLayer != NULL) {
    text_layer_set_text_color(dateLayer, colors[4]);
  }

  if (infoLayer != NULL) {
    text_layer_set_text_color(infoLayer, colors[4]);
  }

  configChanged = true;
  now = time(NULL);
  setHM(localtime(&now));

  //	layer_mark_dirty(rootLayer);
}

static void logVariables(const char *msg) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s\n USDate=%d\n showWeekday=%d\n bigMinutes=%d", msg, USDate, showWeekday, bigMinutes);
  APP_LOG(APP_LOG_LEVEL_DEBUG, " showDate=%d\n curLang=%d\n negative=%d, themecode=%s", showDate, curLang, negative, themeCodeText);
  APP_LOG(APP_LOG_LEVEL_DEBUG, " btalert=%d", btalert);
}

static bool checkAndSaveInt(int *var, int val, int key) {
  if (*var != val) {
    *var = val;
    persist_write_int(key, val);
    return true;
  } else {
    return false;
  }
}

bool checkAndSaveString(char *var, char *val, int key) {
  int ret;

  if (strcmp(var, val) != 0) {
    strcpy(var, val);
    ret = persist_write_string(key, val);
    if (ret < (int)strlen(val)) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "checkAndSaveString() : persist_write_string(%d, %s) returned %d",
              key, val, ret);
    }
    return true;
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "checkAndSaveString() : value has not changed (was : %s, is : %s)",
            var, val);
    return false;
  }
}


static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Message Dropped");
}

static void in_received_handler(DictionaryIterator *received, void *context) {
  bool somethingChanged = false;

  Tuple *dateorder = dict_find(received, CONFIG_KEY_DATEORDER);
  Tuple *weekday = dict_find(received, CONFIG_KEY_WEEKDAY);
  Tuple *lang = dict_find(received, CONFIG_KEY_LANG);
  Tuple *bigminutes = dict_find(received, CONFIG_KEY_BIGMINUTES);
  Tuple *showdate = dict_find(received, CONFIG_KEY_SHOWDATE);
  Tuple *neg = dict_find(received, CONFIG_KEY_NEGATIVE);
  Tuple *bt = dict_find(received, CONFIG_KEY_BTALERT);
  Tuple *themeCodeTuple = dict_find(received, CONFIG_KEY_THEMECODE);

  if (dateorder && weekday && lang && bigminutes && showdate && neg && themeCodeTuple) {
    somethingChanged |= checkAndSaveInt(&USDate, dateorder->value->int32, CONFIG_KEY_DATEORDER);
    somethingChanged |= checkAndSaveInt(&showWeekday, weekday->value->int32, CONFIG_KEY_WEEKDAY);
    somethingChanged |= checkAndSaveInt(&curLang, lang->value->int32, CONFIG_KEY_LANG);
    somethingChanged |= checkAndSaveInt(&bigMinutes, bigminutes->value->int32, CONFIG_KEY_BIGMINUTES);
    somethingChanged |= checkAndSaveInt(&showDate, showdate->value->int32, CONFIG_KEY_SHOWDATE);
    somethingChanged |= checkAndSaveInt(&negative, neg->value->int32, CONFIG_KEY_NEGATIVE);
    somethingChanged |= checkAndSaveInt(&btalert, bt->value->int32, CONFIG_KEY_BTALERT);
    somethingChanged |= checkAndSaveString(themeCodeText, themeCodeTuple->value->cstring, CONFIG_KEY_THEMECODE);

    logVariables("ReceiveHandler");

    if (somethingChanged) {
      applyConfig();
    }
  }
}

static void readConfig() {
  if (persist_exists(CONFIG_KEY_DATEORDER)) {
    USDate = persist_read_int(CONFIG_KEY_DATEORDER);
  } else {
    USDate = 1;
  }

  if (persist_exists(CONFIG_KEY_WEEKDAY)) {
    showWeekday = persist_read_int(CONFIG_KEY_WEEKDAY);
  } else {
    showWeekday = 1;
  }

  if (persist_exists(CONFIG_KEY_LANG)) {
    curLang = persist_read_int(CONFIG_KEY_LANG);
  } else {
    curLang = LANG_ENGLISH;
  }

  if (persist_exists(CONFIG_KEY_BIGMINUTES)) {
    bigMinutes = persist_read_int(CONFIG_KEY_BIGMINUTES);
  } else {
    bigMinutes = 0;
  }

  if (persist_exists(CONFIG_KEY_SHOWDATE)) {
    showDate = persist_read_int(CONFIG_KEY_SHOWDATE);
  } else {
    showDate = 1;
  }

  if (persist_exists(CONFIG_KEY_NEGATIVE)) {
    negative = persist_read_int(CONFIG_KEY_NEGATIVE);
  } else {
    negative = 0;
  }

  if (persist_exists(CONFIG_KEY_BTALERT)) {
    btalert = persist_read_int(CONFIG_KEY_BTALERT);
  } else {
    btalert = 1;
  }

  if (persist_exists(CONFIG_KEY_THEMECODE)) {
    persist_read_string(CONFIG_KEY_THEMECODE, themeCodeText, sizeof(themeCodeText));
  } else {
    strcpy(themeCodeText, "c0fef8c0fd");
    persist_write_string(CONFIG_KEY_THEMECODE, themeCodeText);
  }
  decodeThemeCode(themeCodeText);

  logVariables("readConfig");

}

static void app_message_init(void) {
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_open(128, 128);
}

static void createInfoLayer(int what) {
  static BatteryChargeState chargeState;
  static char text[] = "batt 100%";
  GRect b;

  switch (what) {
    case BATTERY:
      chargeState = battery_state_service_peek();
      snprintf(text, sizeof(text), "batt %d%%", chargeState.charge_percent);
      break;

    case BLUETOOTH_ON:
      snprintf(text, sizeof(text), "BT on");
      break;

    case BLUETOOTH_OFF:
      snprintf(text, sizeof(text), "BT off");
      break;
  }

  if (!showDate) {
    vOffset = VOFFSET_DATE;
    b = layer_get_bounds(bgLayer);
    b.origin.y = vOffset;
    layer_set_bounds(bgLayer, b);
  }

  infoLayer = text_layer_create(GRect(-20, 134, SCREENW+40, TEXTH));
  text_layer_set_background_color(infoLayer, colors[0]);
  text_layer_set_font(infoLayer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(infoLayer, GTextAlignmentCenter);
  text_layer_set_text_color(infoLayer, colors[4]);
  text_layer_set_text(infoLayer, text);
  layer_add_child(rootLayer, text_layer_get_layer(infoLayer));
}

static void destroyInfoLayer(void *data) {
  GRect b;
  if (!showDate) {
    vOffset = VOFFSET_NODATE;
    b = layer_get_bounds(bgLayer);
    b.origin.y = vOffset;
    layer_set_bounds(bgLayer, b);
  }

  text_layer_destroy(infoLayer);
  infoLayer = NULL;
}

static void handle_bluetooth(bool connected) {
  if (lastBluetoothStatus == connected) {
    return;
  } else {
    lastBluetoothStatus = connected;

    if (btalert) {
      if (infoLayer == NULL) {
        if (connected) {
          createInfoLayer(BLUETOOTH_ON);
          vibes_double_pulse();
        } else {
          createInfoLayer(BLUETOOTH_OFF);
          vibes_long_pulse();
        }
        app_timer_register(4000, destroyInfoLayer, NULL);
      }
    }
  }
}

static void handle_tap(AccelAxisType axis, int32_t direction) {
  if (infoLayer == NULL) {
    createInfoLayer(BATTERY);
    app_timer_register(4000, destroyInfoLayer, NULL);
  }
}


// init handler
static void handle_init() {
  int i;

  readConfig();
  app_message_init();

  setColors();

  // Main Window
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, colors[0]);

  // Get root layer
  rootLayer = window_get_root_layer(window);

  // Load custom font
  customFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BORIS_37));

  // Set clock format
  clock12 = !clock_is_24h_style();

  // Big digits Background layer, used to trigger redraws
  bgLayer = layer_create(GRect(0, vOffset, SCREENW, 168-vOffset));
  layer_set_update_proc(bgLayer, bgLayerUpdateProc);
  layer_add_child(rootLayer, bgLayer);

  // Big digits structures & layers, childs of bgLayer
  for (i=0; i<2; i++) {
    bigSlot[i].curDigit = -1;
    bigSlot[i].bitmap = NULL;
  }

  // Small digits TextLayers
  for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
    smallDigitLayer[i] = text_layer_create(GRect(TEXTX+dx[i], vOffset+TEXTY+dy[i], TEXTW, TEXTH));
    text_layer_set_background_color(smallDigitLayer[i], GColorClear);
    text_layer_set_font(smallDigitLayer[i], customFont);
    if (bigMinutes) {
      text_layer_set_text_alignment(smallDigitLayer[i], GTextAlignmentLeft);
    } else {
      text_layer_set_text_alignment(smallDigitLayer[i], GTextAlignmentRight);
    }
    text_layer_set_text_color(smallDigitLayer[i], textColor[i]);
    text_layer_set_text(smallDigitLayer[i], smallDigits);
    layer_add_child(bgLayer, text_layer_get_layer(smallDigitLayer[i]));
  }

  if (showDate) {
    // Date TextLayer
    createDateLayer();
  }

  // Init with current time
  now = time(NULL);
  setHM(localtime(&now));

  applyConfig();

  // Register for minutes ticks
  tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

  // Register tap handler
  accel_tap_service_subscribe(handle_tap);

  // Register Bluetooth handler
  lastBluetoothStatus = bluetooth_connection_service_peek();
  bluetooth_connection_service_subscribe(handle_bluetooth);
}

// deinit handler
static void handle_deinit() {
  int i;

  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();

  if (showDate) {
    destroyDateLayer();
  }

  for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
    text_layer_destroy(smallDigitLayer[i]);
  }
  
  for (i=0; i<2; i++) {
    if (bigSlot[i].bitmap != NULL) {
      gbitmap_destroy(bigSlot[i].bitmap);
    }
  }
  
  layer_destroy(bgLayer);
  
  fonts_unload_custom_font(customFont);
  
  window_destroy(window);
}

// Main
int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
