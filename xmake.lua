add_rules('mode.debug', 'mode.release')

includes('HybridECS/src')

target('HybridECS')
    set_languages('c++23')
    set_kind('binary')
    add_files('HybridECS/src/*.cpp')


includes('HybridECS/test_util')
target('Test')
    set_languages('c++23')
    set_kind('binary')
    add_files('Test/src/*.cpp')