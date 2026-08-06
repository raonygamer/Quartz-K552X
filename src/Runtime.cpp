using InitFunction = void (*)();

extern "C" {
    extern InitFunction __preinit_array_start[]; // NOLINT(*-reserved-identifier)
    extern InitFunction __preinit_array_end[];   // NOLINT(*-reserved-identifier)

    extern InitFunction __init_array_start[]; // NOLINT(*-reserved-identifier)
    extern InitFunction __init_array_end[];   // NOLINT(*-reserved-identifier)

    void __libc_init_array() noexcept  // NOLINT(*-reserved-identifier)
    {
        for (InitFunction* function = __preinit_array_start;
             function != __preinit_array_end;
             ++function) {
                (*function)();
             }

        for (InitFunction* function = __init_array_start;
             function != __init_array_end;
             ++function) {
                (*function)();
             }
    }
}