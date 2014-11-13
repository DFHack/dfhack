file_compressorst& operator=(const file_compressorst &in) {
    compressed = in.compressed;
    /* fstream cannot be assigned */
    in_buffer = in.in_buffer;
    in_buffersize = in.in_buffersize;
    in_buffer_amount_loaded = in.in_buffer_amount_loaded;
    in_buffer_position = in.in_buffer_position;
    out_buffer = in.out_buffer;
    out_buffersize = in.out_buffersize;
    out_buffer_amount_written = in.out_buffer_amount_written;
    return *this;
}
