

T = $(wildcard t[0-9][0-9][0-9][0-9]-*.sh)

all: $(T)

$(T):
	@./$@ $(CGIT_TEST_OPTS)

clean:
	$(RM) -rf trash

.PHONY: $(T) clean
