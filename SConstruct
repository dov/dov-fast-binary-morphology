env = Environment(CPPFLAGS=['-O2','-Wall'],
                  CXXFLAGS=['-std=c++11'],
                  CFLAGS=['-std=c99']
                  )
env.ParseConfig('pkg-config --cflags --libs libpng')

env.Program('test-morph',
            ['test-morph.c',
             'bimg.c',
             'morph.c'])
