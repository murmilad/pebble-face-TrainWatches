#include <pebble.h>
#include "train.h"

#define TRAIN_IMAGES_COUNT 11  

#define KEY_TRAIN_TITLE 0
#define KEY_TRAIN_TIME 1
#define KEY_TRAIN_COUNT 2
#define KEY_TRAIN_NUMBER 3
#define KEY_SHEDULE_SENT 4
#define KEY_COMMAND 5

typedef struct {
  char title[63];
  time_t time;
} Train;

static Train *s_shedule_array;

static BitmapLayer *s_train_layer;
static GBitmap *s_train_bitmap[TRAIN_IMAGES_COUNT];
static uint16_t s_train_index  = 0;
static uint16_t s_trains_count = 0;
static uint8_t s_shedule_received = 0;

static void send_command(char * command) {
  DictionaryIterator* dictionaryIterator = NULL;
  app_message_outbox_begin (&dictionaryIterator);
  dict_write_cstring (dictionaryIterator, KEY_COMMAND, command);
  dict_write_end (dictionaryIterator);
  app_message_outbox_send ();
}

void update_train() {
  
  time_t current_time = time(NULL);

  if (s_shedule_received == 1){
    for (uint16_t i = 0; i < s_trains_count; i++){
      if (current_time + 60 * 10 < s_shedule_array[i].time){
        APP_LOG(APP_LOG_LEVEL_INFO, "Current train is %s", s_shedule_array[i].title);
        APP_LOG(APP_LOG_LEVEL_INFO, "train time %d", (int) s_shedule_array[i].time);
        APP_LOG(APP_LOG_LEVEL_INFO, "current  time %d", (int) current_time);
  
        s_train_index = i;
        uint32_t time_difference = s_shedule_array[i].time - current_time;

        APP_LOG(APP_LOG_LEVEL_INFO, "time_difference %d", (int) time_difference);

        if (time_difference > 60 * 19) {
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[9]);
        } else {
          uint8_t index = time_difference / 60 - 10;
          APP_LOG(APP_LOG_LEVEL_INFO, " get image %d", index);
          bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[index]);
        }
  
        break;
      }
    }
  
    if(current_time % (60 * 60 * 24) == 0) {
      send_command("get_shedule");
    }
  } else {
    bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
  }
}

void train_load(Window *window) {
    // Create GBitmap, then set to created BitmapLayer
  s_train_bitmap[10] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_THINK_LOAD);
  
  s_train_bitmap[9] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN);
  s_train_bitmap[8] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_1);
  s_train_bitmap[7] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_2);
  s_train_bitmap[6] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_3);
  s_train_bitmap[5] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_4);
  s_train_bitmap[4] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_5);
  s_train_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_6);
  s_train_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_7);
  s_train_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_8);
  s_train_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TRAIN_9);

  s_train_layer = bitmap_layer_create(GRect(83, 25, 61, 61));
  bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_train_layer));
}

void train_unload(){
    for(int i = 0; i < TRAIN_IMAGES_COUNT; i++) {
      gbitmap_destroy(s_train_bitmap[i]);
    }

    bitmap_layer_destroy(s_train_layer);
    free(s_shedule_array);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Read first item
  Tuple *t = dict_read_first(iterator);

  int train_number = -1;
  Train train;

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
      case KEY_TRAIN_COUNT:
        APP_LOG(APP_LOG_LEVEL_INFO, "Trains count %d", s_trains_count);

        s_train_index = 0;
        s_shedule_received = 0;
        bitmap_layer_set_bitmap(s_train_layer, s_train_bitmap[10]);
        s_trains_count = t->value->uint16;

        free(s_shedule_array);
        s_shedule_array = (Train*)malloc(s_trains_count * sizeof(Train));
      
        send_command("send_next_train");
        break;
      case KEY_TRAIN_TITLE:
        APP_LOG(APP_LOG_LEVEL_INFO, "tit %s", train.title);

        snprintf(train.title, sizeof(train.title), "%s", t->value->cstring);

        break;
      case KEY_TRAIN_TIME:
        APP_LOG(APP_LOG_LEVEL_INFO, "tim %d", (int) train.time);

        train.time = (time_t) t->value->uint32;

        break;
      case KEY_TRAIN_NUMBER:
        APP_LOG(APP_LOG_LEVEL_INFO, "num %d", (int) train_number);

        train_number = t->value->uint16;

        break;
      case KEY_SHEDULE_SENT:
        APP_LOG(APP_LOG_LEVEL_INFO, "sent %d", (int) s_shedule_received);

        s_shedule_received = (uint8_t)t->value->uint8;
        update_train();

        break;
      default:
        APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
        break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  if (train_number >= 0) {
    s_shedule_array[train_number] = train;
    APP_LOG(APP_LOG_LEVEL_INFO, "train time %d", (int) s_shedule_array[train_number].time);
    send_command("send_next_train");
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void train_init(){
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

}