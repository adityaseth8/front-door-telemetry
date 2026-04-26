
#ifndef WRITE_TO_FAST_API_H
#define WRITE_TO_FAST_API_H

#include <stddef.h>

#define BATCH_SIZE 200
#define JSON_SIZE  160

struct SensorData {
	float x, y, z;
	char  timestamp_us[64];
};

void format_timestamp(char *out, size_t out_size);
struct SensorData receive_sensor_data();
int build_json(float x, float y, float z, char *json, size_t json_size, char *timestamp);
void add_to_batch(const char *json);
void send_batch_data(const char *json);

#endif // WRITE_TO_FAST_API_H
