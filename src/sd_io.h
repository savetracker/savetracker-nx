#ifndef SAVETRACKER_SD_IO_H
#define SAVETRACKER_SD_IO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct SdReader SdReader;
typedef struct SdWriter SdWriter;

bool sd_io_init(void);
void sd_io_exit(void);

bool sd_io_read(const char *path, uint8_t **out_data, size_t *out_size, size_t max_size);
void sd_io_free(uint8_t *buf);

SdReader *sd_io_open_read(const char *path, size_t *out_size);
size_t    sd_io_reader_read(SdReader *r, uint8_t *buf, size_t cap);
void      sd_io_reader_close(SdReader *r);

SdWriter *sd_io_open_write(const char *path);
bool      sd_io_writer_write(SdWriter *w, const uint8_t *data, size_t size);
bool      sd_io_writer_close(SdWriter *w);  // renames .tmp → final
void      sd_io_writer_abort(SdWriter *w);  // removes .tmp, no rename

bool sd_io_write(const char *path, const uint8_t *data, size_t size);
bool sd_io_log(const char *path, const char *msg);

#endif
