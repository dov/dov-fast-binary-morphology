# A trivial makefile for demoing fast binary morphology

CFLAGS = `pkg-config --cflags libpng`
LDFLAGS = `pkg-config --libs libpng`

test-morph : test-morph.c morph.c bimg.c
	$(CC) $(CFLAGS) -o $@ $+ $(LDFLAGS)

clean :
	$(RM) test-morph
