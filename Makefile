.PHONY: compile
compile:
	pio run

.PHONY: upload
upload:
	pio run -t upload

.PHONY: check
check:
	pio check

.PHONY: test
test:
	pio test -e native -v
