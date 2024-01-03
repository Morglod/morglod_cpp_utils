#pragma once

struct file_stream_writer;

void file_stream_writer_init(file_stream_writer* writer);
void file_stream_writer_push_file(file_stream_writer* writer, const char* file_path);
void file_stream_writer_wait_open_read_all(file_stream_writer* writer);
void file_stream_writer_wait_open_all(file_stream_writer* writer);
void file_stream_writer_close(file_stream_writer* writer);
void file_stream_writer_free(file_stream_writer* writer);
