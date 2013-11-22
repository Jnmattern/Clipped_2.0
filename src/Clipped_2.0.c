#include <pebble.h>

// Languages
#define LANG_DUTCH 0
#define LANG_ENGLISH 1
#define LANG_FRENCH 2
#define LANG_GERMAN 3
#define LANG_SPANISH 4
#define LANG_PORTUGUESE 5
#define LANG_MAX 6

enum {
	CONFIG_KEY_DATEORDER	= 90,
	CONFIG_KEY_WEEKDAY		= 91,
	CONFIG_KEY_LANG 		= 92,
	CONFIG_KEY_BIGMINUTES 	= 93,
	CONFIG_KEY_SHOWDATE 	= 94
};

// Compilation flags
#define SMALLDIGITS_WHITE FALSE

// Screen dimensions
#define SCREENW 144
#define SCREENH 168
#define CX      72
#define CY      84

// Date Layer Frame
#define TEXTX 1
#define TEXTY 96
#define TEXTW 142
#define TEXTH 60

// Space between big digits
#define BIGDIGITS_PADDING 6


#define VOFFSET_DATE 0
#define VOFFSET_NODATE 15

int vOffset = VOFFSET_DATE;

// IDs of the images for the big digits
const int digitImage[10] = {
    RESOURCE_ID_IMAGE_D0, RESOURCE_ID_IMAGE_D1, RESOURCE_ID_IMAGE_D2, RESOURCE_ID_IMAGE_D3,
    RESOURCE_ID_IMAGE_D4, RESOURCE_ID_IMAGE_D5, RESOURCE_ID_IMAGE_D6, RESOURCE_ID_IMAGE_D7,
    RESOURCE_ID_IMAGE_D8, RESOURCE_ID_IMAGE_D9
};

// Days of the week in all languages
const char weekDay[LANG_MAX][7][3] = {
	{ "zon", "maa", "din", "woe", "don", "vri", "zat" },		// Dutch
	{ "sun", "mon", "tue", "wed", "thu", "fri", "sat" },		// English
	{ "dim", "lun", "mar", "mer", "jeu", "ven", "sam" },		// French
	{ "son", "mon", "die", "mit", "don", "fre", "sam" },		// German
	{ "dom", "lun", "mar", "mie", "jue", "vie", "sab" },		// Spanish
	{ "dom", "seg", "ter", "qua", "qui", "sex", "sab" }		// Portuguese
};

char buffer[256] = "";
int curLang = LANG_ENGLISH;
int showWeekday = 1;
int USDate = 1;
int showDate = 1;
int bigMinutes = 0;

int configChanged = false;

// Structure to hold informations for the two big digits
typedef struct {
    // the Layer for the digit
    BitmapLayer *layer;
	// the bitmap to display
	GBitmap *bitmap;
    // the frame in which the layer is positionned
    GRect frame;
    // Current digit to display
    int curDigit;
    // Previous digit displayed
    int prevDigit;
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
// The custom font
GFont customFont;
// String for the small digits
char smallDigits[] = "00";
// String for the date
char date[] = "000000";
// various compute variables
char D[2], M[2];
int wd;
// Is clock in 12h format ?
bool clock12 = false;
// x&y offsets for the SMALLDIGITSLAYERS_NUM TextLayers for the small digits to simulate outlining
int dx[SMALLDIGITSLAYERS_NUM] = { -2, 2, 2, -2, 0 };
int dy[SMALLDIGITSLAYERS_NUM] = { -2, -2, 2, 2, 0 };

// Text colors for the SMALLDIGITSLAYERS_NUM TextLayers, depending on SMALLDIGITS_WHITE
#if SMALLDIGITS_WHITE
GColor textColor[SMALLDIGITSLAYERS_NUM] = { GColorBlack, GColorBlack, GColorBlack, GColorBlack, GColorWhite };
#else
GColor textColor[SMALLDIGITSLAYERS_NUM] = { GColorWhite, GColorWhite, GColorWhite, GColorWhite, GColorBlack };
#endif

// Current and previous timestamps, last defined to -1 to be sure to update at launch
time_t now;
struct tm *curTime, last = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, "" };

// big digits update procedure
void updateBigDigits(int val) {
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
			bitmap_layer_set_bitmap(bigSlot[i].layer, bigSlot[i].bitmap);
			
            bigSlot[i].frame = bigSlot[i].bitmap->bounds;
        }
        // Calculate the total width of the two digits so to center them afterwards:
        // they can be different widths so they're not aligned to the center of the screen
        width += bigSlot[i].frame.size.w;
    }

    // Offset the first digit to the left of half the calculated width starting from the middle of the screen
    bigSlot[0].frame.origin.x = CX - width/2;
    // Offset the second digit to the right of the first one
    bigSlot[1].frame.origin.x = bigSlot[0].frame.origin.x + bigSlot[0].frame.size.w + BIGDIGITS_PADDING;

    // foreach big digit slot
    for (i=0; i<2; i++) {
        // Apply offsets
        layer_set_frame(bitmap_layer_get_layer(bigSlot[i].layer), bigSlot[i].frame);
    }
}

void updateSmallDigits(int val) {
    int i;

    smallDigits[0] = '0' + (char)(val/10);
    smallDigits[1] = '0' + (char)(val%10);
    
    for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
        // Set small digits TextLayers's text, this triggers a redraw
        text_layer_set_text(smallDigitLayer[i], smallDigits);
    }
}

// global time variables handler
void setHM(struct tm *tm) {
	int h;
	
	if ((tm->tm_hour != last.tm_hour) || configChanged) {
		h= tm->tm_hour;
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
				date[0] = weekDay[curLang][wd][0];
				date[1] = weekDay[curLang][wd][1];
				date[2] = weekDay[curLang][wd][2];
				date[3] = ' ';
				date[4] = '0' + D[0];
				date[5] = '0' + D[1];
				date[6] = (char)0;
			} else {
				if (USDate) {
					// US date formatting : "mm dd"
					date[0] = '0' + M[0];
					date[1] = '0' + M[1];
					date[2] = ' ';
					date[3] = '0' + D[0];
					date[4] = '0' + D[1];
				} else {
					// EU date formatting : "dd mm"
					date[0] = '0' + D[0];
					date[1] = '0' + D[1];
					date[2] = ' ';
					date[3] = '0' + M[0];
					date[4] = '0' + M[1];
				} // if (USDate)
				date[5] = (char)0;
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
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
	setHM(tick_time);
}

void createDateLayer(void) {
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
		text_layer_set_text_color(dateLayer, GColorWhite);
		text_layer_set_text(dateLayer, date);
		layer_add_child(rootLayer, text_layer_get_layer(dateLayer));
	}
}

void destroyDateLayer(void) {
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

void applyConfig() {
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
	
	configChanged = true;
	now = time(NULL);
	setHM(localtime(&now));
	
	//	layer_mark_dirty(rootLayer);
}

void logVariables(const char *msg) {
	snprintf(buffer, 256, "MSG: %s\n\tUSDate=%d\n\tshowWeekday=%d\n\tbigMinutes=%d\n\tshowDate=%d\n\tcurLang=%d\n", msg, USDate, showWeekday, bigMinutes, showDate, curLang);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, buffer);
}

bool checkAndSaveInt(int *var, int val, int key) {
	if (*var != val) {
		*var = val;
		persist_write_int(key, val);
		return true;
	} else {
		return false;
	}
}

void in_dropped_handler(AppMessageResult reason, void *context) {
}

void in_received_handler(DictionaryIterator *received, void *context) {
	bool somethingChanged = false;

	Tuple *dateorder = dict_find(received, CONFIG_KEY_DATEORDER);
	Tuple *weekday = dict_find(received, CONFIG_KEY_WEEKDAY);
	Tuple *lang = dict_find(received, CONFIG_KEY_LANG);
	Tuple *bigminutes = dict_find(received, CONFIG_KEY_BIGMINUTES);
	Tuple *showdate = dict_find(received, CONFIG_KEY_SHOWDATE);
	
	if (dateorder && weekday && lang && bigminutes && showdate) {
		somethingChanged |= checkAndSaveInt(&USDate, dateorder->value->int32, CONFIG_KEY_DATEORDER);
		somethingChanged |= checkAndSaveInt(&showWeekday, weekday->value->int32, CONFIG_KEY_WEEKDAY);
		somethingChanged |= checkAndSaveInt(&curLang, lang->value->int32, CONFIG_KEY_LANG);
		somethingChanged |= checkAndSaveInt(&bigMinutes, bigminutes->value->int32, CONFIG_KEY_BIGMINUTES);
		somethingChanged |= checkAndSaveInt(&showDate, showdate->value->int32, CONFIG_KEY_SHOWDATE);
		
		logVariables("ReceiveHandler");
		
		if (somethingChanged) {
			applyConfig();
		}
	}
}

void readConfig() {
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
	
	logVariables("readConfig");
	
}

static void app_message_init(void) {
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_open(64, 64);
}


// init handler
void handle_init() {
    int i;
   
    // Main Window
    window = window_create();
    window_stack_push(window, true /* Animated */);
    window_set_background_color(window, GColorBlack);

	app_message_init();
	readConfig();
	
	// Get root layer
	rootLayer = window_get_root_layer(window);
	
    // Load custom font
    customFont = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BORIS_37));
    
    // Set clock format
	clock12 = !clock_is_24h_style();

    // Big digits Background layer, used to trigger redraws
    bgLayer = layer_create(GRect(0, vOffset, SCREENW, 168-vOffset));
    layer_add_child(rootLayer, bgLayer);
    
    // Big digits structures & layers, childs of bgLayer
    for (i=0; i<2; i++) {
        bigSlot[i].curDigit = -1;

        bigSlot[i].layer = bitmap_layer_create(GRect(72*i, 0, 72, 138));
		bigSlot[i].bitmap = NULL;
        layer_add_child(bgLayer, bitmap_layer_get_layer(bigSlot[i].layer));
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

	// Register for minutes ticks
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

// deinit handler
void handle_deinit() {
	int i;
	
	tick_timer_service_unsubscribe();

	if (showDate) {
		text_layer_destroy(dateLayer);
	}
	
	for (i=0; i<SMALLDIGITSLAYERS_NUM; i++) {
		text_layer_destroy(smallDigitLayer[i]);
	}
	
	for (i=0; i<2; i++) {
		gbitmap_destroy(bigSlot[i].bitmap);
		bitmap_layer_destroy(bigSlot[i].layer);
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
