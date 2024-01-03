# file_bulk_reader

```cpp

// append each file in list with this data
struct file_bulk_reader_t_ext {
    std::string extract_to;
};

main() {
    int items_count = 10;

    file_bulk_reader<file_bulk_reader_t_ext> reader;
    reader.init(items_count);

    // init extract_to for each file here
    reader.set_file_data(0, file_bulk_reader_t_ext{
        .extract_to = "./example_package/"
    });
    // ...

    for (... < items_count) {
        // push paths to files
        reader.push_file(tar_path.c_str());
    }

    // now we are ready to read them
    reader.wait_open_read_all(&reader);

    // release file descriptors
    reader.close_fds(&reader);

    for (... reader.num_files) {
        auto& file = reader.file_bufs[i];
        // do smth with data
        (void)file.buf, file.size;
    }
}
```
