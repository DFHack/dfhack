void zoom_display(df::zoom_commands command) {
    SDL_SemWait(async_zoom.sem);
    async_zoom.queue.push_back(command);
    SDL_SemPost(async_zoom.sem);
    SDL_SemPost(async_zoom.sem_fill);
}
