add_rules('mode.debug', 'mode.release')

target('HybridECS')
    set_languages('c++23')
    set_kind('binary')
    add_files('HybridECS/src/*.cpp')
    